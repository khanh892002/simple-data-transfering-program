# simple-data-transfering-program

Để sử dụng chương trình, ta cần biên dịch chương trình ra các file SendData.exe và ReceiveData.exe từ SendData.c và ReceiveData.c
Để biên dịch:
* Trong Windows: ta có thể sử dụng Development Command Prompt để biên dịch các file, hoặc cài đặt MinGW để sử dụng gcc (cho file c) và g++ (cho file cpp).
* Trong Linux: ta có thể sử dụng gcc (có sẵn trong các package thiết yếu của hệ thống).

Ta cần khởi chạy một file ReceiveData.exe trong một terminal riêng giữ vai trò là một server
Sau đó, có thể sử dụng một hoặc nhiều terminal khác để chạy SendData.exe gửi tin nhắn và file đến server, nhưng cần chỉ định rõ địa chỉ IP của server (nếu cùng một máy thì có thể sử dụng địa chỉ 127.0.0.1)
