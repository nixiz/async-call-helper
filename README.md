# Async Call Helper

## Overview

The Async Call Helper is a C++ library designed to simplify the management of object lifetimes and enhance the safety and reliability of handling asynchronous call responses. It provides a generic solution for managing the lifetimes of objects that make asynchronous calls, ensuring that your application remains stable even when calling objects are deleted.

This library is particularly useful in scenarios where both C and C++ are used, especially in places where asynchronous calls are made. It eliminates the need to manually manage object lifetimes and reduces the risk of application crashes due to deleted objects.

## Features

- Automatic management of object lifetimes during asynchronous calls.
- Safe handling of asynchronous call responses.
- Minimal changes required in the classes that use the `async_call_helper` interface.
- Customizable response handling using function pointers like lambdas or `bind`.

## Usage

### Installation

To use the Async Call Helper library in your project, you can clone this repository or download the source code and include it in your build system.

### Integration

1. Include the `async_call_helper.h` header file in your project.
2. Extend your class by inheriting `async_call_helper`.

   ```cpp
   class MyService : public async_call_helper<MyService> {
       // ...
   };
   ```

3. Implement the `execute` method in your class, which will make the asynchronous call.

   ```cpp
   void MyService::execute() {
      // Make your asynchronous call here
      // Pass the result to response_cb
      auto context = get_context<int>([this](int result) {
        std::cout << "result: " << result << "\n";
      });
      c_long_async_function(context.context, context.callback, *param);
   }
   ```

4. You can handle the asynchronous call response directly where you make the call or also forward to response function by using `std::bind`.

   ```cpp
   void MyService::response(int result) {
       // Process the response here
   }
   ```

### Example

For a complete example of how to use the Async Call Helper, check out the [sample code](./async-call-helper/safe-service.cpp).

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contributing

We welcome contributions from the community. If you encounter any issues or have ideas for improvements, please open an issue or submit a pull request.

## Acknowledgments

- The Async Call Helper library was inspired by the need for safer asynchronous call handling in C++ projects.

---

**Disclaimer:** This library is provided as-is, without any warranties or guarantees. Use it at your own risk.

For questions or support, contact [Your Name](mailto:oguzhankatli@gmail.com).
