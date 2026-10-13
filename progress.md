# Progress — Ứng dụng & luồng xử lý qua từng chương

Tổng hợp các chương trình "ứng dụng" trong repo: mỗi bài gồm **sơ đồ**, **luồng xử lý**,
**điểm mấu chốt**, **ưu/nhược điểm** và **điểm cần để ý** trong code hiện tại.

| Chương | Thư mục | Chủ đề |
|---|---|---|
| 2 | [2-Chap2/](2-Chap2/) | Ôn tập C trên UNIX: `system`, `fork`, xử lý xâu, tạo HTML |
| 3 | [3-Chap3/](3-Chap3/) | Socket cơ bản: phân giải tên miền, TCP, UDP |
| 4 | [4-Chap4/](4-Chap4/) | Các mô hình server: iterative, non-blocking, đa tiến trình |

Lộ trình theo [1-Docs/note.md](1-Docs/note.md): blocking → non-blocking → **multi-processing** →
multi-threading → multiplexing.

---

## Mục lục

- [Chương 2 — Ôn tập C trên UNIX](#chương-2--ôn-tập-c-trên-unix)
  - [2.1. Đọc kết quả lệnh shell (ex4, ex5)](#21-đọc-kết-quả-lệnh-shell-ex4-ex5)
  - [2.2. Ghép chuỗi động `Append` (ex6)](#22-ghép-chuỗi-động-append-ex6)
  - [2.3. `fork` + `exec` + `wait` (ex7, lab12)](#23-fork--exec--wait-ex7-lab12)
  - [2.4. Trình duyệt thư mục (lab13)](#24-trình-duyệt-thư-mục-lab13)
- [Chương 3 — Socket cơ bản](#chương-3--socket-cơ-bản)
  - [3.1. Phân giải tên miền (domain2ip)](#31-phân-giải-tên-miền-domain2ip)
  - [3.2. TCP echo một lần (tcp_server / tcp_client)](#32-tcp-echo-một-lần-tcp_server--tcp_client)
  - [3.3. Telnet server (telnet_server / telnet_client)](#33-telnet-server-telnet_server--telnet_client)
  - [3.4. Command server ls / ifconfig (exercise2)](#34-command-server-ls--ifconfig-exercise2)
  - [3.5. UDP sender / receiver](#35-udp-sender--receiver)
  - [3.6. Music server qua UDP (music_server / music_client)](#36-music-server-qua-udp-music_server--music_client)
- [Chương 4 — Các mô hình server](#chương-4--các-mô-hình-server)
  - [4.1. HTTP file server — iterative (html.c)](#41-http-file-server--iterative-htmlc)
  - [4.2. Non-blocking server](#42-non-blocking-server)
  - [4.3. Chat room server (non-blocking)](#43-chat-room-server-non-blocking)
  - [4.4. Đa tiến trình — các ví dụ fork (L3 ex1–ex5)](#44-đa-tiến-trình--các-ví-dụ-fork-l3-ex1ex5)
  - [4.5. Forking server](#45-forking-server)
- [Tổng kết — so sánh các mô hình server](#tổng-kết--so-sánh-các-mô-hình-server)

---

# Chương 2 — Ôn tập C trên UNIX

Chương này chưa có mạng, nhưng các kỹ thuật ở đây được dùng lại trực tiếp ở chương 3–4:
`system()` → telnet server, `Append` → HTTP server, `fork/wait` → forking server.

## 2.1. Đọc kết quả lệnh shell (ex4, ex5)

```
 system("ls -al > ls.txt") ──▶ ls.txt ──fgets từng dòng──▶ sscanf tách 9 cột ──▶ in "tên: kích thước"
```

**Luồng xử lý:**
1. `system()` chạy lệnh shell, chuyển hướng output vào tệp `ls.txt`.
2. `fopen` tệp đó, đọc từng dòng bằng `fgets`.
3. Dựa vào ký tự đầu: `d` là thư mục, `-` là tệp thường.
4. Với tệp thường, `sscanf` tách 9 trường (quyền, số link, owner, group, size, tháng, ngày, giờ, tên) rồi in tên và kích thước.

**Điểm mấu chốt:** đây là cách "lấy output của một chương trình khác" đơn giản nhất. Telnet
server ở 3.3 dùng y nguyên kỹ thuật này. `popen()` ở 3.4 là phiên bản gọn hơn, không cần tệp trung gian.

**Điểm cần để ý:**
- `while (!feof(f))` kiểm tra EOF **trước** khi đọc, nên vòng lặp chạy thêm một lần với dòng rỗng. Cách chuẩn: `while (fgets(line, sizeof line, f) != NULL)`.
- `sscanf("%s")` dừng ở khoảng trắng, nên tên tệp có dấu cách sẽ bị cắt.

## 2.2. Ghép chuỗi động `Append` (ex6)

```
 *output ──realloc(oldlen + strlen(str) + 1)──▶ sprintf nối str vào cuối ──▶ cập nhật *output
```

**Luồng xử lý:**
1. Nhận `char**` để có thể thay đổi con trỏ của người gọi (con trỏ tới con trỏ, lab04).
2. Tính độ dài mới, `realloc` mở rộng vùng nhớ (lần đầu `NULL` thì `realloc` hoạt động như `malloc`).
3. Ghi `str` vào cuối, cập nhật lại `*output`.

**Điểm mấu chốt:** cho phép xây một chuỗi HTML/HTTP **không biết trước độ dài**. Hàm này được
dùng lại nguyên văn trong [4-Chap4/html.c](4-Chap4/html.c) để dựng header và body HTTP.

**Điểm cần để ý:** không kiểm tra `realloc` trả về `NULL`. Mỗi lần nối lại gọi `strlen` toàn chuỗi,
nên nối n lần tốn O(n²). Với trang nhỏ thì không đáng kể.

## 2.3. `fork` + `exec` + `wait` (ex7, lab12)

```
        Cha ──fork()──┬──▶ Con: getpid/getppid ──execl("/bin/echo")──▶ _exit(127) nếu exec lỗi
                      │
                      └──▶ Cha: wait(&status) ──▶ WIFEXITED / WEXITSTATUS / WIFSIGNALED
```

**Luồng xử lý (lab12):**
1. `system("ls -l | head -3")`: thực chất là `fork` + `exec("/bin/sh -c ...")` + `wait` gộp lại.
2. `fflush(stdout)` **trước** `fork()`, để bộ đệm không bị nhân đôi sang con.
3. `fork()`: con thay chính nó bằng chương trình khác qua `execl`. Nếu `execl` trả về nghĩa là lỗi, nên `_exit(127)`.
4. Cha `wait()` rồi phân tích `status`: thoát bình thường (`WEXITSTATUS`) hay bị signal giết (`WTERMSIG`).

ex7 minh hoạ cùng ý: cha in 100 dòng trong 100 giây, con in một dòng rồi thoát. Trong lúc đó con
là **zombie** cho tới khi cha `wait(NULL)`.

**Điểm mấu chốt:** đây là bộ khung của forking server (4.5): cha lặp `fork()`, con làm việc rồi thoát,
cha thu dọn con.

## 2.4. Trình duyệt thư mục (lab13)

```
 ┌──────────── vòng lặp lệnh ────────────┐
 │ ip   → system("ifconfig | grep inet") │
 │ ls   → system("ls -a -l")             │
 │ html → scandir() → listing.html       │
 │ <tên>→ chdir(tên)   ..→ chdir("..")   │
 │ q    → thoát                          │
 └───────────────────────────────────────┘
```

**Luồng xử lý:**
1. Chạy lần lượt 4 phần đề bài: hiển thị IP, `ls -a -l`, xuất HTML, rồi vào chế độ tương tác.
2. Mỗi vòng đọc một lệnh từ bàn phím, so sánh bằng `strcmp`, gọi hàm tương ứng.
3. `scandir(".", &ds, loc, alphasort)` lấy danh sách đã sắp xếp. Mỗi mục được ghi thành `<a href>` trong `listing.html`.
4. `..` bị chặn khi đang ở `/`.

**Điểm mấu chốt:** đây là phiên bản **offline** của HTTP file server ở 4.1. Thay "đọc lệnh từ bàn
phím + ghi HTML ra tệp" bằng "đọc request từ socket + gửi HTML qua socket" là ra 4.1.

---

# Chương 3 — Socket cơ bản

Ứng với slide **Chapter 3** (socket, sockaddr, byte order, `getaddrinfo`) và phần đầu
**Chapter 4** (TCP client/server, UDP `sendto`/`recvfrom`).

Trình tự gọi hàm chung:

```
 TCP Server: socket → bind → listen → accept ─┐       TCP Client: socket → connect
                                              ├── recv/send ──┤
                                     close ◀──┘               └──▶ close

 UDP Server: socket → bind → recvfrom ⇄ sendto           UDP Client: socket → sendto ⇄ recvfrom
             (không listen/accept, không có "kết nối")
```

## 3.1. Phân giải tên miền (domain2ip)

```
 "google.com" ──getaddrinfo()──▶ danh sách addrinfo ──duyệt ai_next──┬─ AF_INET  → inet_ntoa
                                                                    └─ AF_INET6 → inet_ntop
```

**Luồng xử lý:**
1. Lấy tên miền từ `argv[1]` (mặc định `google.com`).
2. `getaddrinfo(tên, "http", NULL, &res)` hỏi DNS, trả về **danh sách liên kết** các địa chỉ.
3. Duyệt danh sách, theo `ai_family` mà ép về `sockaddr_in` (IPv4) hoặc `sockaddr_in6` (IPv6) để in.
4. `freeaddrinfo(res)` giải phóng.

**Điểm mấu chốt:** `getaddrinfo` là API mới thay cho `gethostbyname`, hỗ trợ cả IPv4 lẫn IPv6
(slide 33). Một tên miền thường có nhiều IP, nên kết quả là danh sách chứ không phải một giá trị.

**Điểm cần để ý:** khi thiếu tham số, code gán `argv[1] = "google.com"`. Chạy được (vì `argv[argc]` là `NULL`
và còn chỗ), nhưng nên dùng biến riêng: `const char *host = argc > 1 ? argv[1] : "google.com";`.

## 3.2. TCP echo một lần (tcp_server / tcp_client)

```
   Client                         Server (port 8080)
   socket                         socket → bind → listen
   connect ─────── SYN ─────────▶ accept
   send "Hello, Server!" ───────▶ recv → in ra
   recv ◀──────── echo ───────── send lại đúng dữ liệu
   close                          close client, close listener → THOÁT
```

**Luồng xử lý:**
1. Server tạo socket, `bind` vào `INADDR_ANY:8080`, `listen`.
2. `accept` **một** kết nối, `recv` một lần, gửi trả lại (echo).
3. Đóng cả hai socket và kết thúc.

**Điểm mấu chốt:** đây là bộ khung tối thiểu. `htons()` bắt buộc cho port vì mạng dùng big-endian.
Thêm `'\0'` sau `recv` vì dữ liệu nhận không tự có ký tự kết thúc.

**Nhược:** chỉ phục vụ đúng 1 client rồi thoát. Bài sau thêm vòng `while` quanh `accept`.

## 3.3. Telnet server (telnet_server / telnet_client)

Bài tập slide 20 Chapter 4: nhận lệnh từ client, thực thi, gửi kết quả về.

```
   Client (8888)                       Server (iterative)
   connect ───────────────────────▶   accept
   recv "Simple Telnet Server" ◀────  send lời chào
   fgets lệnh → send ─────────────▶   recv → cắt \r\n → nối "> out.txt"
                                       system(lệnh) → out.txt
   recv lặp tới khi 0 ◀────────────   fopen out.txt → send từng dòng
   close                       ◀────   close(c) → quay lại accept
```

**Luồng xử lý:**
1. Server `listen` trên cổng 8888, vòng lặp `accept` vô tận (**iterative server**).
2. Gửi lời chào, `recv` lệnh, xoá `\r\n` ở cuối.
3. Nối `"> out.txt"` rồi `system()`, giống kỹ thuật 2.1.
4. Đọc `out.txt`, gửi từng dòng về client, đóng kết nối.
5. Client đọc **cho tới khi `recv` trả về 0** (server đóng). Đây là cách đơn giản để client biết "hết dữ liệu".

**Ưu:** ngắn, tái dùng được mọi lệnh shell có sẵn.
**Nhược:** mỗi lần chỉ phục vụ 1 client; client chậm gõ lệnh thì mọi client khác phải chờ.

**Điểm cần để ý:**
- **Lỗi bảo mật nghiêm trọng:** chuỗi từ client đi thẳng vào `system()`, nên client gửi `rm -rf ~` là server chạy luôn. Chỉ dùng để học.
- Nếu client đóng ngay (`recv` trả về 0), `buffer` rỗng, `strlen(buffer) - 1` thành số rất lớn nên truy cập ngoài mảng. Cần kiểm tra `received <= 0` trước.
- Hai client cùng lúc sẽ cùng ghi `out.txt`. Ở đây chưa xảy ra vì server iterative, nhưng sẽ gặp khi chuyển sang forking server.
- `clen` phải là `socklen_t` (gcc cảnh báo `-Wpointer-sign`).

## 3.4. Command server ls / ifconfig (exercise2)

```
   nc localhost 8081                   Server (iterative, port 8081)
   "ls\n" ───────────────────────▶   recv → strcspn cắt \r\n → so sánh lệnh
                                       ├─ "ls"       → opendir/readdir → ghép tên → send
                                       ├─ "ifconfig" → popen("ifconfig") → send từng dòng
                                       └─ khác       → "Invalid command"
   ◀──────────── kết quả ───────────   close(client) → quay lại accept
```

**Luồng xử lý:**
1. Tách xử lý thành `handle_client` → `handle_ls` / `handle_ifconfig` (bảng điều phối lệnh, giống lab05).
2. `ls` tự liệt kê bằng `opendir/readdir`, **không gọi shell**.
3. `ifconfig` dùng `popen()` để đọc thẳng output của lệnh, không cần tệp tạm như 3.3.
4. `accept` lỗi thì `continue` chứ không thoát server.

**Ưu so với 3.3:** chỉ chấp nhận danh sách lệnh cố định nên an toàn hơn. Kiểm tra lỗi đầy đủ (`perror`).
**Nhược:** vẫn iterative; mỗi kết nối chỉ một lệnh; kết quả `ls` bị cắt nếu vượt 4096 byte.

## 3.5. UDP sender / receiver

```
   udp_sender                         udp_receiver (port 9090)
   socket(SOCK_DGRAM)                 socket(SOCK_DGRAM) → bind
   fgets → sendto(127.0.0.1:9090) ──▶ recvfrom → in ra
   (lặp)                              (lặp)
```

**Luồng xử lý:** sender không `bind`, không `connect`. Mỗi `sendto` mang theo địa chỉ đích.
Receiver chỉ cần `bind` rồi `recvfrom` liên tục.

**Điểm mấu chốt (TCP vs UDP):**
- UDP **giữ ranh giới gói**: mỗi `sendto` tương ứng đúng một `recvfrom`. TCP là dòng byte nên một `send` có thể bị tách hoặc gộp khi `recv`.
- UDP không đảm bảo tới nơi, không đảm bảo thứ tự. Gửi khi receiver chưa chạy thì gói mất, không báo lỗi.

**Điểm cần để ý:** [udp_demo.c](3-Chap3/udp_demo.c) đang viết dở. `addr.sin_addr.s_addr = inet_addr;` gán
**tên hàm** thay vì gọi hàm, gcc báo lỗi `-Wint-conversion`, file không biên dịch được.

## 3.6. Music server qua UDP (music_server / music_client)

Một **giao thức tự định nghĩa** trên UDP, có lệnh, gói điều khiển và báo lỗi.

```
   Client                                   Server (port 9090, ./server_playlist)
   "GETLIST" ────────────────────────────▶ opendir → mỗi tệp thường 1 datagram
             ◀── "a.mp3" ◀── "b.mp3" ◀────
             ◀── "END_OF_LIST" ◀──────────

   "DOWNLOAD a.mp3" ─────────────────────▶ chặn ".." và "/" → fopen
             ◀── block 10KB ◀── block ◀────  fread 10KB → sendto (lặp)
             ◀── "END_OF_FILE" ◀──────────

   lệnh sai ─────────────────────────────▶ "ERROR Unknown command: ..."
   (mọi recvfrom có timeout 3 giây bằng SO_RCVTIMEO)
```

**Luồng xử lý:**
1. Server `bind` cổng 9090. Mỗi `recvfrom` lấy luôn địa chỉ client vào `client_socket`, dùng để trả lời đúng người hỏi.
2. **GETLIST:** mỗi tên bài là một datagram riêng, kết thúc bằng gói `END_OF_LIST`.
3. **DOWNLOAD:** kiểm tra tên (chống path traversal), đọc tệp từng khối 10 KB, gửi mỗi khối một datagram, kết thúc bằng `END_OF_FILE`.
4. **Client** đặt `SO_RCVTIMEO` 3 giây. Hết giờ thì `recvfrom` trả về -1 với `EAGAIN`, coi như mất kết nối, xoá tệp tải dở.
5. Client kiểm tra gói điều khiển **trước** khi `fwrite`, để `END_OF_FILE` không lẫn vào dữ liệu nhạc.

**Ưu:** không cần kết nối, server không giữ trạng thái nên một server trả lời được nhiều client lần lượt.
Minh hoạ rõ cách thiết kế giao thức: lệnh, dấu kết thúc, mã lỗi, timeout.

**Nhược / điểm cần để ý:**
- UDP có thể **mất, trùng, đảo thứ tự** gói. Server gửi liên tục không chờ ACK nên dễ tràn bộ đệm nhận phía client, tệp tải về bị thiếu mà client không biết. Muốn tin cậy cần đánh số block + ACK/gửi lại (hoặc dùng TCP).
- Datagram 10 KB vượt MTU 1500 byte nên bị phân mảnh IP. Trên loopback không sao, nhưng qua mạng thật chỉ cần mất 1 mảnh là mất cả khối.
- Trong lúc server đang gửi một bài, yêu cầu của client khác phải chờ (server đơn luồng).

---

# Chương 4 — Các mô hình server

Ứng với slide **Chapter 4** phần "TCP Server types" (slide 13): *iterating server* và *forking server*,
cùng bài thực hành non-blocking.

## 4.1. HTTP file server — iterative (html.c)

```
   Trình duyệt                              Server (port 8080)
   GET /documents/ HTTP/1.1 ─────────────▶ accept → recv 1 lần → sscanf method, uri
                                            ├─ method ≠ GET     → 400 Bad Request
                                            ├─ uri chứa ".."    → 404
                                            ├─ stat lỗi         → 404 Not Found
                                            ├─ là thư mục       → list_directory: <ul><li><a href>…
                                            └─ là tệp           → send_file: header + fread/send 4KB
            ◀── HTTP/1.1 200 OK + body ────
                                            close(client) → quay lại accept
```

**Luồng xử lý:**
1. `SO_REUSEADDR` để khởi động lại server ngay, không bị lỗi `bind: Address already in use`.
2. Vòng lặp `accept`. Mỗi kết nối chỉ `recv` một lần, lấy dòng đầu `GET /path HTTP/1.1`.
3. Ánh xạ URI sang đường dẫn tệp: `/` thành `.`, `/x` thành `./x`.
4. `stat` để phân biệt thư mục / tệp:
   - **Thư mục:** dùng `Append` (2.2) dựng trang HTML, mỗi mục là một liên kết. Bấm vào thư mục con sẽ tạo request mới.
   - **Tệp:** gửi header `Content-Type: application/octet-stream`, `Content-Disposition: attachment` (trình duyệt tải về), `Content-Length`, rồi gửi nội dung theo khối 4 KB.
5. `Connection: close`: mỗi request một kết nối, gửi xong thì đóng.

**Điểm mấu chốt:** một phản hồi HTTP gồm **dòng trạng thái + header + dòng trống `\r\n\r\n` + body**.
`Content-Length` giúp trình duyệt biết khi nào đã nhận đủ.

**Ưu:** chạy được với trình duyệt thật, dễ kiểm tra (`http://localhost:8080/`).
**Nhược:** iterative. Một client tải tệp lớn hoặc kết nối mà không gửi gì sẽ chặn tất cả client khác ở `recv`.

**Điểm cần để ý:**
- Tên tệp có dấu cách hoặc tiếng Việt bị trình duyệt mã hoá thành `%20`... Server không giải mã nên trả về 404.
- `send` có thể gửi thiếu byte với tệp lớn. Cần vòng lặp gửi đủ (như `ghi_het()` ở lab08).
- Server phục vụ thư mục đang đứng khi chạy (`.`). Hãy chạy trong `4-Chap4/` để thấy thư mục `documents/`.

## 4.2. Non-blocking server

```
                    ┌──────────────── vòng lặp vô tận (không bao giờ ngủ) ────────────────┐
                    │ accept(listener)  ── -1 (EWOULDBLOCK): chưa có ai → bỏ qua          │
                    │                   ── fd mới: FIONBIO, thêm vào client_sockets[]     │
                    │ for mỗi client:                                                     │
                    │    recv ── > 0 : in ra, send "Hello from server!"                   │
                    │         ── 0   : client ngắt → close                                │
                    │         ── -1  : chưa có dữ liệu → client tiếp theo                 │
                    └─────────────────────────────────────────────────────────────────────┘
```

**Luồng xử lý:**
1. `ioctl(listener, FIONBIO, &1)` chuyển socket sang **non-blocking**: `accept`/`recv` trả về ngay với `-1` (`errno = EWOULDBLOCK`) thay vì đứng chờ.
2. Mỗi client mới cũng được đặt non-blocking và lưu vào mảng (tối đa 10).
3. Một vòng lặp duy nhất lần lượt "hỏi thăm" listener và từng client (**polling**).

**Điểm mấu chốt:** một tiến trình, một luồng nhưng phục vụ được nhiều client cùng lúc, vì không lệnh
nào bị chặn. Đây là bước đầu tiên trong lộ trình (blocking → non-blocking).

**Ưu:** đơn giản, không tốn tiến trình/luồng; dữ liệu các client nằm chung một chỗ.
**Nhược:** **busy-waiting**: vòng lặp chạy liên tục kể cả khi không có gì, chiếm 100% một nhân CPU.
Mô hình multiplexing (`select`/`poll`) ở phần sau giải quyết đúng điểm này: để kernel báo socket nào sẵn sàng.

**Điểm cần để ý:**
- Khi client ngắt (`recv == 0`), code `close()` nhưng **không xoá khỏi mảng**. Các vòng sau `recv` trên fd đã đóng sẽ trả về -1 (`EBADF`) mãi, `num_clients` không giảm. Tệ hơn, `accept` sau đó có thể cấp lại đúng số fd đó, làm một client bị phục vụ hai lần. [chat_room_server.c](4-Chap4/chat_room_server.c) đã sửa đúng chỗ này.
- `buffer` không được gán `'\0'` sau `recv` nên có thể in lẫn dữ liệu cũ.
- `recv == -1` chưa phân biệt `EWOULDBLOCK` (bình thường) với lỗi thật như `ECONNRESET`.

## 4.3. Chat room server (non-blocking)

```
   Client A ──"hi"──▶ ┌──────────────────────┐ ──"hi"──▶ Client B
                      │  Server non-blocking │ ──"hi"──▶ Client C
                      │  client_sockets[]    │
   Client B ◀──────── └──────────────────────┘   (gửi cho mọi người TRỪ người gửi)
```

**Luồng xử lý:** giống 4.2, khác ở hai chỗ:
1. Nhận được tin từ client `i` thì **broadcast**: `send` cho mọi `j != i`.
2. Client ngắt kết nối: `close`, **dồn mảng** lên một ô, `num_clients--`, `i--` để không bỏ sót phần tử vừa dồn lên.

**Điểm mấu chốt:** chat room cần **dữ liệu dùng chung** (danh sách mọi client). Mô hình một tiến trình
non-blocking làm việc này rất tự nhiên. Forking server (4.5) thì khó, vì mỗi con có bộ nhớ riêng,
con A không biết socket của con B.

**Ưu:** dùng chung danh sách client dễ dàng, không cần IPC.
**Nhược:** vẫn busy-waiting 100% CPU như 4.2.

**Điểm cần để ý:**
- `recv(..., sizeof(buffer))` rồi `buffer[byte_received] = '\0'`: nếu nhận đủ 1024 byte thì ghi `'\0'` **ra ngoài mảng**. Nên `recv` với `sizeof(buffer) - 1`.
- Client bị lỗi (`ECONNRESET`) rơi vào nhánh `-1` nên không bao giờ bị xoá khỏi danh sách.
- `send` trên socket non-blocking có thể gửi thiếu hoặc trả về -1 khi bộ đệm đầy. Ở đây bỏ qua.

## 4.4. Đa tiến trình — các ví dụ fork (L3 ex1–ex5)

| Bài | Kỹ thuật | Quan sát |
|---|---|---|
| [ex1](4-Chap4/L3-MultiProcessing/ex1.c) | `fork()` và giá trị trả về (`0` = con, `>0` = cha) | 2 tiến trình, 2 dòng in |
| [ex2](4-Chap4/L3-MultiProcessing/ex2.c) | 2 `fork()` liên tiếp | 2² = 4 tiến trình; code trước `fork` chạy 1 lần |
| [ex3](4-Chap4/L3-MultiProcessing/ex3.c) | `exit()` + `wait()` + `WEXITSTATUS` | con thành **zombie** từ giây 5 đến giây 20 (`ps` thấy `<defunct>`) |
| [ex4](4-Chap4/L3-MultiProcessing/ex4.c) | tạo 5 con trong vòng `for`, `killpg(0, SIGKILL)` | con lặp vô tận nên không fork tiếp; cha giết cả nhóm sau 10 s |
| [ex5](4-Chap4/L3-MultiProcessing/ex5.c) | `signal(SIGCHLD, handler)` | cha không bị chặn; handler tự `wait()` khi con chết |

**Điểm cần để ý:** ex3 thiếu `#include <sys/wait.h>`. Với gcc trên máy này đây là **lỗi biên dịch**
(`implicit declaration of function 'wait'`), không chỉ là cảnh báo.

## 4.5. Forking server

Đây là mô hình "mỗi client một tiến trình", kết hợp tất cả kỹ thuật ở 4.4:

```
        ┌────────────── Cha (listener) ──────────────┐
        │  socket → bind → listen → accept (lặp mãi)  │
        └──────────┬──────────────┬──────────────────┘
        accept #1  │   accept #2  │
              fork │         fork │
              ┌────▼───┐     ┌────▼───┐
              │ Con 1  │     │ Con 2  │   mỗi con phục vụ 1 client
              └────────┘     └────────┘
```

**Luồng xử lý:**
1. Cha tạo socket, `bind`, `listen`, rồi lặp `accept()`. Mỗi kết nối mới trả về một socket `client` riêng.
2. Sau `accept`, cha `fork()`.
3. **Con** `close(listener)` vì không cần nhận kết nối mới. Con dùng `client` để `recv` và `send` với client đó, rồi `exit(0)` khi client ngắt kết nối (`recv <= 0`).
4. **Cha** `close(client)` vì socket đó giờ là việc của con. Cha quay lại `accept` để đón client tiếp theo.
5. `SIGCHLD` handler thu dọn các con đã xong, tránh zombie.

**Điểm mấu chốt:** hai lệnh `close` rất quan trọng. Sau `fork()`, cả cha lẫn con đều giữ bản sao của
mọi file descriptor. Socket chỉ thật sự đóng khi **mọi** bản sao đều được đóng. Nếu cha quên
`close(client)`, cha sẽ rò rỉ descriptor, và client sẽ không bao giờ nhận được tín hiệu ngắt kết nối.

**Ưu:** đơn giản, dễ viết. Mỗi client chạy độc lập với bộ nhớ riêng. Một con bị crash không làm sập
server hay các client khác. Không busy-waiting: con nào không có dữ liệu thì bị chặn ở `recv`, không tốn CPU.
**Nhược:** `fork()` tốn tài nguyên (bộ nhớ, thời gian tạo), không phù hợp với hàng nghìn client.
Các tiến trình có bộ nhớ riêng nên chia sẻ dữ liệu giữa client rất khó. Chat room (4.3) theo mô hình
này cần IPC (pipe, shared memory...).

**Điểm cần để ý:**
- Thiếu `#include <sys/wait.h>`, nên **không biên dịch được** với gcc hiện tại (giống ex3).
- `buffer` không được gán `'\0'` sau `recv`, nên `printf("%s")` có thể in rác. Nên gán `buffer[bytes_recved] = 0` (và `recv` với `sizeof(buffer) - 1`).
- Handler chỉ `wait()` một lần. Nhiều con chết cùng lúc thì `SIGCHLD` bị gộp, sót zombie. Nên dùng `while (waitpid(-1, NULL, WNOHANG) > 0);`.
- Không kiểm tra `accept` trả về -1. Nếu lỗi, server vẫn `fork` một con với fd không hợp lệ.
- Áp dụng cho các bài trước: bọc telnet server (3.3) hay HTTP server (4.1) bằng đúng khung này là phục vụ được nhiều client cùng lúc. Riêng telnet server cần đổi `out.txt` thành tên riêng theo PID, hoặc dùng `popen`.

---

# Tổng kết — so sánh các mô hình server

| Mô hình | Bài trong repo | Nhiều client cùng lúc? | CPU khi rảnh | Chia sẻ dữ liệu giữa client | Chi phí mỗi client |
|---|---|---|---|---|---|
| Iterative (blocking) | tcp_server, telnet_server, exercise2, html.c | Không, xếp hàng | ~0% (chặn ở `accept`/`recv`) | — | Thấp nhất |
| Non-blocking polling | non_blocking_server, chat_room_server | Có | **100%** (busy-wait) | Dễ (cùng bộ nhớ) | Thấp |
| Multi-process (fork) | forking_server | Có | ~0% | Khó (cần IPC) | Cao (1 tiến trình) |
| UDP (không kết nối) | music_server, udp_receiver | Lần lượt từng datagram | ~0% | Dễ | Thấp |
| *Multi-threading* | *chưa có — buổi tiếp theo* | Có | ~0% | Dễ nhưng cần mutex | Trung bình (1 luồng) |
| *Multiplexing (select/poll)* | *chưa có* | Có | ~0% | Dễ | Thấp |

Mỗi bước trong lộ trình giải quyết nhược điểm của bước trước:

```
Iterative ──(client chậm chặn tất cả)──▶ Non-blocking ──(tốn 100% CPU)──▶ Multi-process
    ──(fork tốn kém, khó chia sẻ)──▶ Multi-thread ──(cần đồng bộ, tốn luồng)──▶ Multiplexing
```
