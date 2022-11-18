# async-call-helper
C ve C++ dillerinin ortak kullanıldığı projelerde, özellikle asenkron çağrıların yapıldığı yerler başta olmak üzere bir
çok noktada nesnelerin ömürlerinin yönetilmesi zor ve sorunlu olabilmektedir. Bunun sebebi, asenkron çağrıların
yapıldığı yerlerde, geribildirimin yapılacağı nesnelerin yaşam sürelerinin otomatik olarak uzatılmıyor olmasıdır.

Örneğin, aşağıdaki koda baktığımız zaman, C kütüphanesindeki bir asenkron çağrıyı yapan `service` objesinin asenkron
çağrı sonuçlanmadan hafızadan silinmesi durumunda, çağrının geribildiriminde kendisinin silinip silinmediğini anlaması
imkansızdır. Aşağıda satır 3'te erişim yapılmaya çalışılan `unsafe_service` nesnesi silinmiş olduğu için uygulama çakılacaktır.

```cpp
1. static inline void response_cb(void* context, int response) {
2.  auto srv_ptr = static_cast<unsafe_service*>(context);
3.  srv_ptr->response(response);
4. }
5. 
6. void unsafe_service::execute() {
7.   c_long_async_function((void*)this, response_cb, in_param);
8.   delete this;
9. }
```

### std::enable_shared_from_this&lt;T&gt; Kullanımı

Yukarıdaki örnekte karşılaşılan sorunu çözebilmek için `unsafe_service` sınıfının [`std::enable_shared_from_this<unsafe_service>`][enable-shared-this-link] yardımcı sınıfından türemesi ve sınıfın [`shared_ptr<unsafe_service>`][shared-ptr-link] türünden yaratılarak, kullanılıyor olması gerekmektedir. Sadece bunu yapması da yetmeyecektir, bir de asenkron çağrıya direkt olarak kendi referansı yerine, içerisinde [`weak_ptr`][weak-ptr-link] referansının tutulduğu başka bir yardımcı nesne göndermesi de gerekmektedir. Bu sayede asenkron çağrının cevabı alındığında, context argümanı bu yardımcı nesne türüne dönüştürülecek ve içerisindeki [`weak_ptr<unsafe_service>`][weak-ptr-link] kullanılarak servis objesinin yaşayıp yaşamadığı kontrol edilebilecektir.

```cpp
class unsafe_service 
  : public std::enable_shared_from_this<unsafe_service> {
// ...
};

struct async_callback_token {
  std::weak_ptr<unsafe_service> caller;
};

static inline void response_cb(void* context, int out_param) {
  std::unique_ptr<async_callback_token> act_handle(
      static_cast<async_callback_token*>(context));
  auto srv_ptr_weak = act_handle->caller;
  if (auto srv_ptr = srv_ptr_weak.lock()) {
    srv_ptr->response(out_param);
  }
  else {
    std::cerr << "caller instance is deleted!!\n";
  }
}

void unsafe_service::execute() {
  auto* context = new async_callback_token{
    this->weak_from_this()
  };
  c_long_async_function((void*)context, response_cb, 300, 300);
}
```

{: .box-note}
**Note:** Yukarıdaki kodun tamamına [buradan][godbolt-1] ulaşabilirsiniz.

Bulmuş olduğumuz çözüm çalışıyor ancak, öncelikle geliştirilen sınıf üzerinde değişiklikler yapılmasını ve
yöntemi kullanmak isteyen sınıfların yaşam döngülerinin [`shared_ptr<T>`][shared-ptr-link] türünden yaratılarak yönetilmesini gerektiriyor.
Üzerinde çalıştığımız sınıfları biz geliştiriyor olsak bile, kullanım şekillerini değiştirmemiz pek mümkün olmayabiliyor. Örneğin bir Framework kullanıyorsak ve bu framework `service` sınıfını nasıl yaratıyorsa, biz de o şekilde kullanmak zorundayız. Ayrıca, asenkron çağrılar yapan her sınıf için aynı yöntemi tekrar tekrar uyguluyor olmak, hem zor hem de [SOLID][solid-link-wiki]
prensiplerine aykırı olacaktır.

Bunun yerine, asenkron çağrı yapan sınıfların otomatik olarak asenkron çağrı cevaplarını işleyebiliyor olması, en önemlisi de çağrı yapan nesnelerin sistemden silindiği durumlarda uygulamanın çakılmasına sebep vermeyecek güvenli bir yapının kullanılması çok daha doğru bir yaklaşım olacaktır.

### `async_call_helper` Sınıfının Geliştirilmesi

Yukarıda sorunu tanımladık, çözüm için basit bir geliştirme de yaptık. Geliştirdiğimiz örnek üzerinden genel bir çözüm üretebilmek için gereksinimlerimizi belirleyebiliriz. Geliştireceğimiz asenkron çağrı yardımcı sınıfımız en az aşağıdaki özelliklere sahip olmalıdır:

* kullanılacak sınıfın yaratılma şeklini veya yaşam süresini **değiştirmemeli**
* kullanılacak sınıf üzerinde/içerisinde en az değişikliğe sebep olmalı
* kullanılan sınıf nesnesi sistemden silindiği taktirde, asenkron çağrı cevapları uygulamanın çakılmasına sebep olmamalı
* **bonus:** asenkron çağrı cevapları tercihen [`lambda`][lambda-link] veya [`bind`][bind-link] gibi fonksiyon belirteçleri üzerinden işlenebilmeli

`async_call_helper` sınıfımızı, yukarıda belirtilen gereksinimleri karşılayacak şekilde geliştirmeye başlayalım. Öncelikle, sınıfın nasıl kullanılacağına karar vermemiz gerekiyor. Bu arayüzü kullanacak sınıfların, tanımladığımız özelliklere erişiminin doğal gözüküyor olmasını sağlamak amacıyla `async_call_helper` sınıfının kalıtım ile kullanılması uygun olacaktır. Aynı zamanda, servis sınıflarının türüne ve referansına erişebilmesi için de `async_call_helper` sınıfının [CRTP][crtp-link] tekniğini de kullanması gerekecektir.

```cpp
template <typename Caller>
class async_call_helper
{
public:
  // ...
  void* get_context() const;
};

class safe_service 
  : public async_call_helper<safe_service> {
public:
  void execute();
};

void safe_service::execute() {
  c_long_async_function(get_context(), response_cb, 300, 300);
}
```

[CRTP][crtp-link] tekniğinin kullanımı sayesinde, `safe_service` sınıfı `get_context()` metodunu kendi metoduymuş gibi kullanabilirken, `async_call_helper` sınıfı ise `safe_service` referansına istediği zaman erişebilmektedir.
Sınıfımızı geliştirmeye devam edelim. Örnek çözümümüzde de kullandığımız [`shared_ptr`][shared-ptr-link] ve [`weak_ptr`][weak-ptr-link] ikilisine burada da ihtiyacımız var. `async_call_helper` sınıfını kullanacak sınıfların nasıl yaratılacağını değiştiremeyeceğimiz için, `async_call_helper` sınıfı içerisinde bir [`shared_ptr`][shared-ptr-link] nesnesine ihtiyacımız var.

```cpp
template <typename Caller>
class async_call_helper
{
public:
  void* get_context() const;
  // ...
private:
  struct auto_ref_holder {
    // ...
  };
  std::shared_ptr<auto_ref_holder> lifetime_ref;
};
```

**Note:** `auto_ref_holder` sınıfının `shared_ptr` olarak yaratılıp, kullanılıyor olması `async_call_helper` tarafından gerçekleştiği için, `std::enable_shared_from_this<auto_ref_holder>` sınıfından türemesine gerek yoktur.

Yukarıda yazdığımız örnek koda tekrar bakacak olursak, asenkron çağrıya servis sınıfının kendisi yerine `async_callback_token` ismini verdiğimiz ara bir nesnenin gönderildiğini göreceğiz. Bu yardımcı sınıf sayesinde, servis objesinin güvenli bir şekilde yaşayıp yaşamadığını kontrol edebiliyor, asenkron çağrının cevabının işlendiği fonksiyonda uygulamayı çakılmalardan koruyabiliyoruz. Aynı sınıfı, yeni yazdığımız `async_call_helper` sınıfı içerisinde de kullanmalıyız. `get_context()` fonksiyonu, bu yardımcı sınıfı geri dönüyor olacak.

```cpp
template <typename Caller>
class async_call_helper
{
public:
  async_callback_token* get_context() const;
  // ...
};
```

Asenkron çağrı, `response_cb(void *context, ...)` fonksiyonunu çağırdığında, `context` nesnesi aslında `asyn_call_token` nesnesini barındıracağından, ilk olarak `asyn_call_token` nesnesine dönüşümünü yapacağız. Sonra, bu sınıfın `get_caller()` sanal metodunu çağırarak servis objemizin referansına, ya da eğer bu obje silinmiş ise `nullptr` değişkenine erişebileceğiz. Bu sayede, uygulamada bir çakılmaya neden olmadan güvenli bir şekilde çalıştırabileceğimiz döngüye sahip olacağız.

```cpp
struct asyn_call_token
{
  virtual ~asyn_call_token() = default;
  template <typename Cast> 
  static Cast* from_context(void* context) noexcept
  {
    std::unique_ptr<asyn_call_token> handle(
      reinterpret_cast<asyn_call_token*>(context));
    auto *cast_ptr = static_cast<Cast*>(handle->get_caller());
    return cast_ptr;    
  }
protected:
  virtual void* get_caller() = 0;
};

static inline void response_cb(void* context, int out_param) {
  auto srv_ptr = asyn_call_token::from_context<safe_service>(context);
  if (srv_ptr) {
    srv_ptr->response(out_param);
  } else {
    std::cerr << "service instance has already been deleted\n";
  }
}
```

Yukarıda asenkron çağrının cevabında kullanılacak `context` objesinin nasıl kullanılacağını yazmış olduk. Yazdığımız kodu çalışır hale getirmek için hala `get_context()` metodunun içerisini doldurmamız gerekiyor. Bu metoddan dönecek `asyn_call_token` objesi, içerisinde `auto_ref_holder` sınıfının bir zayıf referansını tutarak, `get_context()` metodu çağırıldığı zaman geriye servis sınıfının referansını, ya da silinmiş ise `nullptr` değerini döndürüyor olmalı.

```cpp
template <typename Caller>
class async_call_helper
{
public:
  async_call_helper() {
    lifetime_ref = std::make_shared<auto_ref_holder>(
      static_cast<Caller*>(this));
  }

  void* get_context() const 
  {
    struct special_token final : public asyn_call_token
    {
      special_token(std::weak_ptr<auto_ref_holder> ref_) 
        : ref(ref_) {}
      ~special_token() = default;

      void* get_caller() override {
        auto sref = ref.lock();
        return (sref) ? sref->get_parent() : nullptr;
      }
    private:
      std::weak_ptr<auto_ref_holder> ref;
    };
    return new special_token(weak_ref());
  }
  // ...
private:
  struct auto_ref_holder
    : public std::enable_shared_from_this<auto_ref_holder> {
    explicit auto_ref_holder(Caller* caller_) : caller(caller_) {}
    Caller* get_parent() { return caller; }
  };
  std::weak_ptr<auto_ref_holder> weak_ref() const noexcept {
    return lifetime_ref;
  }
  std::shared_ptr<auto_ref_holder> lifetime_ref;
};
```

**Note:** `asyn_call_token` sınıfının polimorfik olması sayesinde, ileride ihtiyaca göre farklı gereksinimleri de karşılayabilir bir yapı kurmuş olduk. Örneğin ileride asenkron çağrılarımıza zaman aşımı sayaçları ekleyebiliriz; çağrı X sn boyunca cevaplanmazsa, zaman aşımı geribildirimine sahip olabiliriz.

### We ❤️ Modern C++

Aslında şu ana kadar yazdığımız kadarıyla başta istediğimiz gereksinimleri karşılar duruma geldik. Ancak, sadece C fonksiyonları için değil, C++ asenkron çağrıları için de `async_call_helper` sınıfını kullanabiliriz. Aynı şekilde, Modern C++'ın getirdiği [lambda][lambda-link] ifadeleri sayesinde, asenkron çağrıların cevapları, çağrının yapıldığı yerde yazılarak kodun okunaklığını da bir kat artırabiliriz.

```cpp
void safe_service::execute() 
{
  auto context = get_context<int>([this] (int result) {
    std::cout << "received result" << result << "\n";
    this->process_result_of_async_call(result);
  });
  c_long_async_function(context.context, context.callback, *param);
}
```

Yukarıdaki gibi bir sentaksa sahip bir arayüzün olması, kodun ilk haline kıyasla ne kadar sade ve okunaklı değil mi. Devam edelim ve yukarıdaki kodu çalışır hale getirecek `get_context` metodumuzu yazalım. Yeni metodumuz bir önceki `get_context` metodu ile aynı mekaniklere sahip olacak, bunu yaparken sadece C geribildirim fonksiyonunu kendi içerisinde tutuyor olması gerekecek, ki asenkron çağrıya `context.callback` olarak o çağrının istediği imzaya sahip bir fonksiyon belirteçi verebilsin. Bunu yapmak için, fonksiyon imzasını bildiği bir fonksiyon belirteci*(function pointer)*, `context` objesi ve C++ çağrıları için `opertor()` metodlarına sahip bir `callback_context` objesini dönmesi gerekecek.

```cpp
template <typename ...Args>
struct callback_context {
  void *context;
  void (*callback)(Args...);
  void operator()(Args... args) noexcept {
    std::invoke(callback, std::forward<Args>(args)...);
  }
};
```

`callback_context` objesinin template olması, asenkron çağrıda kullanılacak fonksiyonun argüman tiplerini bilmesine, çağrılara parametre olarak verilecek değişkenlerin doğru imzalara sahip olmasına imkan sağlayacaktır. C dilindeki çağrılar için, `context` ve `callback` değişkenlerini kullanırken, C++ çağrıları için ise `callback_context` nesnesinin kendisini asenkron çağrıya veriyor olmamız yeterli olacaktır.
`callback_context` nesnesini dönecek yeni `get_context` metodumuzu yazmaya başlayalım. İçerisinde bir önceki metod gibi `asyn_call_token` türünü kalıtan bir yardımcı sınıfın olması gerekiyor. Bu yardımcı sınıf, bir önceki sınıftan farklı olarak, asenkron çağrının cevabını işleyecek [`lambda`][lambda-link] ifadeyi veya fonksiyon belirtecini de tutuyor olacak. Asenkron çağrı geri bildirim fonksiyonunu çağırdığı zaman, yazdığımız yeni yardımcı sınıf, servis nesnesi hala yaşıyor ise en başta verilen fonksiyonu çağırıyor olacak.

```cpp
template <typename Caller>
class async_call_helper
{
public:
  template <typename ...Args, typename Fn>
  callback_context<void*, Args...> 
  get_context(Fn&& cb) noexcept 
  {
    struct trampoline_t final
      : public asyn_call_token
    {
      trampoline_t(std::weak_ptr<auto_ref_holder> ref_, 
                   std::function<void(Args...)> callback_) 
        : ref(ref_) 
        , callback(std::move(callback_)) { }
      ~trampoline_t() = default;

      void* get_caller() override {
        guard.lock();
        auto sref = ref.lock();
        return (sref) ? sref->get_parent() : nullptr;
      }

      static inline void callback_handle(void* context, Args... args) {
        std::unique_ptr<trampoline_t> trampoline_ptr(
          reinterpret_cast<trampoline_t*>(context));
        if (trampoline_ptr->get_caller()) {
          std::invoke(trampoline_ptr->callback, std::forward<Args>(args)...);
        }
      }
    private:
      std::weak_ptr<auto_ref_holder> ref;
      std::function<void(Args...)> callback;
    };
    std::function<void(Args...)> callback = cb;
    return callback_context<void*, Args...>
           {
             new trampoline_t(weak_ref(), std::move(callback)),
             &trampoline_t::callback_handle
           };
  }
  // ...
};
```

**Note:** Bu yazıda karmaşıklığı daha da artırmamak için `get_context()` metodu için tür çıkarımı otomasyonu kullanılmamıştır. Tür çıkarımını, bütün olası fonksiyon belirteç varyasyonları için yapan arkadaşımız olursa, buraya kendisinin adıyla birlikte paylaşacağı kodları severek koymak isterim.

Yukarıda statik olarak tanımlanan `callback_handle` fonksiyonu, asenkron çağrının cevabını işleyecek fonksiyon görevini üstlenecek ve asıl kullanılmak istenilen geribildirim fonksiyonunu güvenli olarak çalıştıracak, ya da eğer servis objesi silinmiş ise hiçbir şey yapmayarak uygulamayı çakılmaktan kuratacak. `get_context()` metodu, asenkron çağrının cevabı için kullanacağı fonksiyonun türleri ile çağırılması sayesinde, `callback_context` objesini doğru değişkenler ile yaratabilmektedir.

Bütün bu geliştirmeler sonrasında, asenkron çağrılarımıza verdiğimiz geri bildirim fonksiyonları, çağrıyı yapan nesnenin doğruluğunu kontrol ederek, hem güvenli bir şekilde uygulamanın çakılmasını önlerken, hem de daha modern bir yapıya sahip olmaktayız. Yazdığımız kodları birleştirirsek kodumuzun son hali aşağıdaki gibi olacaktır:

```cpp
struct asyn_call_token
{
  virtual ~asyn_call_token() = default;

  template <typename Cast>
  static inline Cast* from_context(void* context) noexcept
  {
    if (!context) return nullptr;
    std::unique_ptr<asyn_call_token> act_handle(
      reinterpret_cast<asyn_call_token*>(context));
    return static_cast<Cast*>(act_handle->get_caller());
  }
protected:
  virtual void* get_caller() = 0;
};

template <typename Caller>
class async_call_helper
{
public:
  using ThisType = async_call_helper<Caller>;
  ~async_call_helper() = default;
protected:
  async_call_helper() {
    lifetime_ref = std::make_shared<auto_ref_holder>(parent());
  }

  void* get_context() const noexcept 
  {
    struct special_token final
      : public asyn_call_token
    {
      special_token(std::weak_ptr<auto_ref_holder> ref_, std::mutex& guard_) 
        : ref(ref_) 
        , guard(guard_, std::defer_lock) {}
      
      ~special_token() {
        if (guard) {
          guard.unlock();
        }
      }

      void* get_caller() override {
        guard.lock();
        auto sref = ref.lock();
        return (sref) ? sref->get_parent() : nullptr;
      }
    private:
      std::weak_ptr<auto_ref_holder> ref;
      std::unique_lock<std::mutex> guard;
    };
    return new special_token(weak_ref(), guard);
  }
  template <typename ...Args>
  struct callback_context {
    void *context;
    void (*callback)(void*, Args...);
    void operator()(Args... args) noexcept {
      std::invoke(callback, context, std::forward<Args>(args)...);
    }
  };

  template <typename ...Args, typename Fn>
  callback_context<Args...> 
  get_context(Fn&& cb) noexcept 
  {
    struct trampoline_t final
      : public asyn_call_token
    {
      trampoline_t(std::weak_ptr<auto_ref_holder> ref_, 
            std::mutex& guard_,
            std::function<void(Args...)> callback_) 
        : ref(ref_) 
        , guard(guard_, std::defer_lock)
        , callback(std::move(callback_)) {}
      
      ~trampoline_t() {
        if (guard) {
          guard.unlock();
        }
      }

      void* get_caller() override {
        guard.lock();
        auto sref = ref.lock();
        return (sref) ? sref->get_parent() : nullptr;
      }

      static inline void callback_handle(void* context, Args... args) {
        std::unique_ptr<trampoline_t> trampoline_ptr(reinterpret_cast<trampoline_t*>(context));
        if (!trampoline_ptr) return;
        if (trampoline_ptr->get_caller()) {
          std::invoke(trampoline_ptr->callback, std::forward<Args>(args)...);
        }
      }
    private:
      std::weak_ptr<auto_ref_holder> ref;
      std::unique_lock<std::mutex> guard;
      std::function<void(Args...)> callback;
    };
    std::function<void(Args...)> callback = cb;
    return callback_context<Args...> {
      new trampoline_t(weak_ref(), guard, std::move(callback)),
      &trampoline_t::callback_handle
    };
  }
protected:
  Caller* parent() {
    return static_cast<Caller*>(this);
  }

  const Caller* parent() const {
    return static_cast<const Caller*>(this);
  }

  void set_deleted() noexcept {
    std::lock_guard<std::mutex> lock(guard);
    lifetime_ref.reset();
  }

private:
  friend struct auto_ref_holder;
  struct auto_ref_holder
    : public std::enable_shared_from_this<auto_ref_holder> {
    explicit auto_ref_holder(Caller* caller_) : caller(caller_) {}
    Caller* get_parent() {return caller;}
    const Caller* get_parent() const {return caller;}
  private:
    Caller* caller;
  };

  std::weak_ptr<auto_ref_holder> weak_ref() noexcept {
    return lifetime_ref;
  }

  std::weak_ptr<auto_ref_holder> weak_ref() const noexcept {
    return lifetime_ref;
  }
  std::shared_ptr<auto_ref_holder> lifetime_ref;
  mutable std::mutex guard;
};
```

---

[godbolt-1]: https://godbolt.org/z/jfx5E73f4
[shared-ptr-link]: https://en.cppreference.com/w/cpp/memory/shared_ptr
[weak-ptr-link]: https://en.cppreference.com/w/cpp/memory/weak_ptr
[solid-link-wiki]: https://en.wikipedia.org/wiki/SOLID
[lambda-link]: https://en.cppreference.com/w/cpp/language/lambda
[bind-link]: https://en.cppreference.com/w/cpp/utility/functional/bind
[crtp-link]: https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
[enable-shared-this-link]: https://en.cppreference.com/w/cpp/memory/enable_shared_from_this
