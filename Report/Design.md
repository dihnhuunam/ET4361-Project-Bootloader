# Features

## Bootloader

Bootloader là thành phần chạy đầu tiên sau khi MCU reset. Bootloader chịu trách nhiệm kiểm tra trạng thái firmware, chọn application hợp lệ để chạy, hỗ trợ cập nhật firmware, rollback khi firmware mới lỗi và đảm bảo thiết bị không bị brick.

Hệ thống sử dụng thiết kế 2 application slots, còn gọi là A/B firmware update. Tại một thời điểm, một slot là firmware đang hoạt động, slot còn lại là inactive slot dùng để nhận firmware mới.

Bootloader không ghi đè trực tiếp lên firmware đang chạy. Firmware mới luôn được ghi vào inactive slot. Sau khi firmware mới được ghi thành công và kiểm tra CRC hợp lệ, slot mới được đánh dấu là pending. Bootloader sẽ boot thử firmware pending ở lần reset tiếp theo.

Nếu firmware pending chạy ổn định, application sẽ xác nhận firmware đó là confirmed. Nếu firmware pending không xác nhận sau số lần boot thử tối đa, bootloader sẽ rollback về firmware confirmed trước đó.

### Boot Application

Bootloader chọn application để chạy dựa trên metadata và trạng thái của từng application slot.

Bootloader chỉ được phép boot vào một application nếu application đó hợp lệ. Một application hợp lệ cần thỏa mãn các điều kiện:

- Metadata hợp lệ.
- Slot được đánh dấu là pending hoặc confirmed.
- Firmware size không vượt quá dung lượng application slot.
- CRC firmware khớp với CRC lưu trong metadata.
- Stack pointer đầu tiên nằm trong vùng RAM hợp lệ.
- Reset handler nằm trong vùng Flash của application slot tương ứng.

Bootloader ưu tiên boot firmware pending nếu có firmware mới đang chờ xác nhận. Nếu không có firmware pending, bootloader sẽ boot firmware confirmed hiện tại.

Nếu không có firmware hợp lệ, bootloader sẽ vào safe mode để chờ cập nhật firmware qua UART.

### CRC Verification

CRC được sử dụng để kiểm tra tính toàn vẹn của firmware.

Sau khi nhận firmware mới, hệ thống tính CRC trên image đã ghi vào Flash và so sánh với CRC được gửi từ host. Nếu CRC khớp, firmware mới được xem là đã ghi thành công và có thể được đánh dấu pending.

Bootloader cũng kiểm tra CRC trước khi boot vào application để tránh chạy firmware bị lỗi, ghi thiếu hoặc hỏng dữ liệu Flash.

CRC được dùng cho các mục đích:

- Kiểm tra firmware sau khi OTA.
- Kiểm tra application trước khi boot.
- Phát hiện firmware bị ghi lỗi hoặc thiếu dữ liệu.
- Ngăn boot vào firmware không hoàn chỉnh.
- Hỗ trợ anti-brick và rollback.

Firmware có CRC sai không được phép boot.

### Rollback

Rollback là cơ chế quay lại firmware cũ nếu firmware mới không hoạt động đúng.

Firmware mới sau khi OTA không được xem là ổn định ngay lập tức. Firmware mới sẽ được đánh dấu là pending và bootloader sẽ boot thử firmware này.

Application pending phải xác nhận rằng nó đã khởi động và hoạt động ổn định. Sau khi confirm thành công, firmware pending trở thành confirmed.

Nếu application pending không confirm, ví dụ do treo, reset liên tục hoặc lỗi khởi tạo, bootloader sẽ tăng số lần boot thử. Khi số lần boot thử vượt quá giới hạn cho phép, bootloader đánh dấu firmware pending là invalid và rollback về firmware confirmed trước đó.

Rollback giúp thiết bị tiếp tục hoạt động ngay cả khi firmware mới bị lỗi.

### Anti-brick

Anti-brick là mục tiêu quan trọng nhất của thiết kế bootloader 2 slots.

Thiết kế này đảm bảo rằng firmware đang chạy không bị ghi đè trong quá trình OTA. Firmware mới luôn được ghi vào inactive slot. Vì vậy, nếu quá trình OTA bị gián đoạn, mất nguồn hoặc firmware mới không hợp lệ, firmware cũ vẫn còn nguyên và có thể tiếp tục được boot.

Các nguyên tắc anti-brick:

- Không ghi firmware mới vào active slot.
- Chỉ ghi firmware mới vào inactive slot.
- Chỉ đánh dấu firmware mới là pending sau khi ghi đầy đủ và CRC hợp lệ.
- Firmware pending phải confirm trước khi trở thành confirmed.
- Nếu firmware pending không confirm, bootloader rollback về firmware confirmed trước đó.
- Nếu mất nguồn trong lúc OTA, metadata không chuyển sang pending và firmware cũ vẫn được boot.
- Nếu metadata mới bị lỗi, bootloader sử dụng metadata hợp lệ trước đó.
- Nếu cả hai application không hợp lệ, bootloader vào safe mode để chờ OTA qua UART.

### Bootloader UART OTA

Bootloader hỗ trợ nhận firmware qua UART để phục vụ cập nhật firmware và phục hồi thiết bị.

Trong chế độ Bootloader UART OTA, bootloader là thành phần trực tiếp giao tiếp với host, nhận firmware mới, ghi vào inactive slot, kiểm tra CRC và cập nhật metadata.

Bootloader UART OTA có thể được kích hoạt bằng một trong các cách:

- Nhấn nút khi reset.
- Host gửi command trong khoảng thời gian chờ sau reset.
- Application ghi cờ OTA request vào metadata rồi reset.
- Bootloader vào safe mode vì không có application hợp lệ.

Quy trình Bootloader UART OTA:

1. Bootloader vào OTA mode.
2. Bootloader xác định inactive slot.
3. Host gửi thông tin firmware mới qua UART.
4. Bootloader kiểm tra firmware size không vượt quá 232 KB.
5. Bootloader xóa vùng Flash của inactive slot.
6. Bootloader nhận và ghi firmware mới vào inactive slot.
7. Bootloader tính CRC firmware đã ghi.
8. Bootloader so sánh CRC với CRC nhận từ host.
9. Nếu CRC hợp lệ, bootloader cập nhật metadata.
10. Slot mới được đánh dấu là pending.
11. MCU reset.
12. Bootloader boot thử firmware pending.

Nếu OTA thất bại, bootloader không cập nhật metadata sang pending. Firmware confirmed hiện tại vẫn được giữ nguyên.

### Runtime UART OTA From Application

Ngoài chế độ OTA do bootloader xử lý, hệ thống còn hỗ trợ nhận firmware OTA qua UART khi application đang chạy.

Trong cơ chế này, application là thành phần giao tiếp trực tiếp với host qua UART để nhận firmware mới. Firmware mới vẫn luôn được ghi vào inactive slot, không ghi vào active slot hiện tại.

Ví dụ:

- Nếu Application 1 đang chạy, firmware OTA sẽ được ghi vào Application 2.
- Nếu Application 2 đang chạy, firmware OTA sẽ được ghi vào Application 1.

Sau khi application nhận đủ firmware, ghi firmware vào inactive slot và kiểm tra CRC thành công, application cập nhật metadata để đánh dấu slot mới là pending. Sau đó application reset MCU. Ở lần reset tiếp theo, bootloader phát hiện firmware pending, kiểm tra CRC và boot thử firmware mới.

Runtime UART OTA cho phép thiết bị nhận firmware mới khi application đang chạy mà không cần vào bootloader OTA mode ngay từ đầu.

Các chức năng chính của Runtime UART OTA:

- Application nhận OTA command qua UART.
- Application xác định inactive slot.
- Application kiểm tra firmware size không vượt quá 232 KB.
- Application xóa vùng Flash của inactive slot.
- Application nhận firmware theo từng gói dữ liệu.
- Application ghi firmware mới vào inactive slot.
- Application tính CRC firmware đã ghi.
- Application so sánh CRC với CRC được gửi từ host.
- Nếu CRC hợp lệ, application cập nhật metadata và đánh dấu firmware mới là pending.
- Application reset MCU để bootloader xử lý firmware pending.
- Nếu OTA thất bại, application không cập nhật metadata và firmware hiện tại tiếp tục chạy.

Runtime OTA vẫn đảm bảo anti-brick vì active slot hiện tại không bị xóa hoặc ghi đè.

### Metadata Management

Metadata là vùng Flash dùng để lưu trạng thái hệ thống bootloader. Bootloader và application dùng metadata để xác định slot nào đang active, slot nào pending, slot nào confirmed và slot nào cần rollback.

Metadata cần lưu các thông tin chính:

- Magic value để nhận diện metadata hợp lệ.
- Sequence number để xác định bản metadata mới nhất.
- Active slot hiện tại.
- Previous slot dùng cho rollback.
- Pending slot nếu có firmware mới đang chờ xác nhận.
- Trạng thái của Application 1.
- Trạng thái của Application 2.
- Kích thước firmware của từng application.
- CRC của từng application.
- Version của từng application.
- Số lần boot thử firmware pending.
- Giới hạn số lần boot thử tối đa.
- OTA request flag nếu application muốn reset vào bootloader OTA mode.
- CRC của metadata.

Vùng metadata có dung lượng 8 KB. Vì metadata quyết định hành vi boot, vùng này cần được thiết kế có khả năng chống lỗi mất nguồn khi đang cập nhật trạng thái.

Metadata nên được tổ chức thành hai bank dự phòng hoặc nhiều bản ghi tuần tự. Mỗi lần cập nhật metadata, hệ thống ghi một bản mới với sequence number tăng lên. Khi khởi động, bootloader đọc các bản metadata, kiểm tra magic value và CRC metadata, sau đó chọn bản hợp lệ mới nhất.

Cách này giúp tránh lỗi khi mất nguồn trong lúc cập nhật metadata.

---

## Application

Application là firmware chính của thiết bị. Trong demo này, application có chức năng đơn giản để tập trung thể hiện khả năng hoạt động của bootloader.

Application gồm các chức năng chính:

- Blink LED.
- UART log.
- Confirm firmware.
- Runtime UART OTA.
- OTA trigger về bootloader nếu cần.

### Blink LED

Application điều khiển LED để biểu thị trạng thái hoạt động.

Có thể sử dụng tốc độ blink khác nhau giữa Application 1 và Application 2 để dễ quan sát firmware nào đang chạy.

Ý tưởng demo:

- Application 1 blink LED chậm.
- Application 2 blink LED nhanh.
- Firmware lỗi có thể không blink hoặc reset liên tục để demo rollback.

### UART Log

Application gửi log qua UART để hỗ trợ quan sát trạng thái hệ thống trong quá trình demo.

UART log có thể hiển thị:

- Application hiện tại đang chạy.
- Firmware version.
- Slot hiện tại.
- Trạng thái pending hoặc confirmed.
- Thông báo application đã khởi động thành công.
- Thông báo application đã confirm firmware.
- Trạng thái nhận OTA.
- Tiến trình nhận firmware.
- Kết quả kiểm tra CRC.
- Thông báo reset sang bootloader để boot firmware mới.

UART log giúp kiểm tra trực quan quá trình boot, OTA, confirm và rollback.

### Confirm Firmware

Khi một application mới được boot ở trạng thái pending, application phải xác nhận rằng nó đã khởi động và hoạt động ổn định.

Việc confirm firmware nên được thực hiện sau khi application đã khởi tạo thành công các thành phần quan trọng như:

- System clock.
- GPIO.
- UART.
- Logic chính của application.

Sau khi application confirm thành công:

- Slot hiện tại được chuyển từ pending sang confirmed.
- Slot hiện tại trở thành active slot chính.
- Boot attempt counter được reset.
- Pending slot được xóa.
- Firmware trước đó có thể được giữ lại làm rollback slot cho lần cập nhật tiếp theo.

Nếu application không confirm, bootloader sẽ tiếp tục xem firmware đó là pending. Sau một số lần boot thất bại, bootloader sẽ rollback về firmware confirmed trước đó.

### Runtime UART OTA

Application hỗ trợ nhận firmware mới qua UART khi đang chạy. Đây là cơ chế Runtime UART OTA.

Khi nhận được OTA command từ host, application chuyển sang trạng thái OTA receiving. Trong trạng thái này, application nhận firmware theo từng gói dữ liệu, ghi dữ liệu vào inactive slot và theo dõi tiến trình cập nhật.

Application chỉ được phép ghi firmware vào inactive slot. Active slot hiện tại không được xóa hoặc ghi đè để đảm bảo nếu OTA bị lỗi hoặc mất nguồn, firmware hiện tại vẫn còn nguyên.

Sau khi nhận đủ firmware, application kiểm tra CRC toàn bộ image. Nếu CRC hợp lệ, application cập nhật metadata để đánh dấu firmware mới là pending. Sau đó application reset MCU để bootloader boot thử firmware mới.

Nếu CRC sai, dữ liệu thiếu, firmware vượt quá kích thước slot hoặc ghi Flash thất bại, application hủy OTA và giữ nguyên metadata hiện tại. Firmware mới không được đánh dấu pending.

Runtime UART OTA giúp demo trực quan hơn vì người dùng có thể thấy application đang chạy, nhận firmware mới qua UART, sau đó reset và chuyển sang firmware mới.

### OTA Trigger To Bootloader

Ngoài việc tự nhận OTA khi đang chạy, application cũng có thể hỗ trợ cơ chế yêu cầu bootloader vào OTA mode.

Application có thể nhận command qua UART, sau đó ghi OTA request flag vào metadata và reset MCU. Sau reset, bootloader đọc metadata, phát hiện OTA request flag và vào Bootloader UART OTA mode.

Cơ chế này hữu ích khi muốn bootloader trực tiếp nhận firmware thay vì application nhận firmware.

---

# Flash Memory Layout

MCU sử dụng STM32 có tổng dung lượng Flash là 512 KB. Flash được tổ chức thành 256 page, đánh số từ page 0 đến page 255. Mỗi page có kích thước 2 KB.

Flash base address là 0x08000000. Toàn bộ vùng Flash kết thúc tại 0x0807FFFF.

Thiết kế bộ nhớ được chia thành 5 vùng chính:

| Vùng nhớ          |      Page | Địa chỉ bắt đầu | Địa chỉ kết thúc | Kích thước | Mục đích                                                             |
| ----------------- | --------: | --------------: | ---------------: | ---------: | -------------------------------------------------------------------- |
| Bootloader        |    0 - 15 |      0x08000000 |       0x08007FFF |      32 KB | Chạy đầu tiên sau reset, kiểm tra firmware, OTA, rollback, safe mode |
| Metadata          |   16 - 19 |      0x08008000 |       0x08009FFF |       8 KB | Lưu trạng thái bootloader, slot, CRC, version, rollback state        |
| Reserved / Config |   20 - 23 |      0x0800A000 |       0x0800BFFF |       8 KB | Lưu config, boot flags, manufacturing info hoặc future use           |
| Application 1     |  24 - 139 |      0x0800C000 |       0x08045FFF |     232 KB | Application slot 1                                                   |
| Application 2     | 140 - 255 |      0x08046000 |       0x0807FFFF |     232 KB | Application slot 2                                                   |

Tổng dung lượng:

| Thành phần        | Kích thước |
| ----------------- | ---------: |
| Bootloader        |      32 KB |
| Metadata          |       8 KB |
| Reserved / Config |       8 KB |
| Application 1     |     232 KB |
| Application 2     |     232 KB |
| Tổng cộng         |     512 KB |

## Memory Map

```text
0x08000000  +---------------------------+
            | Bootloader                |
            | 32 KB                     |
0x08007FFF  +---------------------------+

0x08008000  +---------------------------+
            | Metadata                  |
            | 8 KB                      |
0x08009FFF  +---------------------------+

0x0800A000  +---------------------------+
            | Reserved / Config         |
            | 8 KB                      |
0x0800BFFF  +---------------------------+

0x0800C000  +---------------------------+
            | Application 1             |
            | 232 KB                    |
0x08045FFF  +---------------------------+

0x08046000  +---------------------------+
            | Application 2             |
            | 232 KB                    |
0x0807FFFF  +---------------------------+
```

## Bootloader Region

Bootloader nằm ở đầu Flash, bắt đầu từ địa chỉ 0x08000000. Đây là vùng được MCU boot vào sau reset.

Bootloader chiếm page 0 đến page 15, tương đương 32 KB. Dung lượng này được dùng cho các chức năng:

- Khởi tạo hệ thống tối thiểu.
- Đọc và kiểm tra metadata.
- Kiểm tra CRC application.
- Chọn application để boot.
- Jump vào application.
- Nhận firmware OTA qua UART.
- Ghi firmware vào inactive slot.
- Cập nhật metadata.
- Xử lý rollback.
- Xử lý anti-brick.
- Vào safe mode khi không có firmware hợp lệ.

Bootloader không được ghi đè trong quá trình OTA thông thường. Firmware OTA chỉ được ghi vào Application 1 hoặc Application 2.

## Metadata Region

Metadata nằm ngay sau bootloader, từ page 16 đến page 19, tổng dung lượng 8 KB.

Metadata dùng để lưu trạng thái hệ thống bootloader. Vì metadata quyết định firmware nào được boot, vùng này cần được bảo vệ khỏi lỗi mất nguồn hoặc ghi lỗi.

Metadata có thể được tổ chức như sau:

|         Page | Mục đích                                           |
| -----------: | -------------------------------------------------- |
| Page 16 - 17 | Metadata Bank A hoặc metadata record area          |
| Page 18 - 19 | Metadata Bank B hoặc metadata record area dự phòng |

Bootloader sẽ đọc metadata, kiểm tra CRC metadata và chọn bản metadata hợp lệ mới nhất dựa trên sequence number.

Metadata nên lưu tối thiểu các thông tin:

| Thông tin            | Mục đích                                  |
| -------------------- | ----------------------------------------- |
| Magic value          | Xác định metadata đã được khởi tạo        |
| Sequence number      | Xác định metadata mới nhất                |
| Active slot          | Slot đang được chọn làm firmware chính    |
| Previous slot        | Slot dùng để rollback                     |
| Pending slot         | Slot firmware mới đang chờ xác nhận       |
| Application state    | Trạng thái từng application               |
| Application size     | Kích thước firmware trong từng slot       |
| Application CRC      | CRC dùng để kiểm tra từng firmware        |
| Application version  | Version firmware                          |
| Boot attempt counter | Số lần đã thử boot firmware pending       |
| Max boot attempts    | Số lần boot thử tối đa trước khi rollback |
| OTA request flag     | Yêu cầu bootloader vào OTA mode           |
| Metadata CRC         | Kiểm tra tính toàn vẹn của metadata       |

## Reserved / Config Region

Reserved / Config nằm từ page 20 đến page 23, tổng dung lượng 8 KB.

Vùng này được dùng để lưu các dữ liệu không thuộc trực tiếp metadata boot decision, ví dụ:

- Device configuration.
- Manufacturing information.
- Device ID.
- Calibration data.
- Boot statistics.
- Crash reason.
- Future extension.

Vùng này giúp tách biệt dữ liệu cấu hình khỏi metadata bootloader, làm cho metadata đơn giản và an toàn hơn.

## Application 1 Region

Application 1 nằm từ page 24 đến page 139, tổng dung lượng 232 KB. Địa chỉ bắt đầu của Application 1 là 0x0800C000.

Application 1 là một firmware slot độc lập. Slot này có thể là firmware đang chạy hoặc là inactive slot để nhận firmware OTA mới, tùy theo trạng thái hiện tại của hệ thống.

Khi boot vào Application 1, vector table của application phải tương ứng với địa chỉ bắt đầu của slot Application 1.

Application 1 có thể chứa:

- Vector table.
- Firmware image.
- Thông tin version.
- Nội dung chương trình blink LED.
- UART log.
- Runtime UART OTA.
- Cơ chế confirm firmware.

## Application 2 Region

Application 2 nằm từ page 140 đến page 255, tổng dung lượng 232 KB. Địa chỉ bắt đầu của Application 2 là 0x08046000.

Application 2 có vai trò tương tự Application 1. Hai application slots được dùng luân phiên cho cơ chế A/B update.

Khi Application 1 đang active, Application 2 là inactive slot và có thể nhận firmware mới. Khi Application 2 đang active, Application 1 trở thành inactive slot.

Application 2 có thể chứa:

- Vector table.
- Firmware image.
- Thông tin version.
- Nội dung chương trình blink LED.
- UART log.
- Runtime UART OTA.
- Cơ chế confirm firmware.

## Linker Configuration Summary

Các địa chỉ sau cần được dùng khi cấu hình linker script hoặc project build:

| Image         | Flash Origin | Flash Length |
| ------------- | -----------: | -----------: |
| Bootloader    |   0x08000000 |        32 KB |
| Application 1 |   0x0800C000 |       232 KB |
| Application 2 |   0x08046000 |       232 KB |

Metadata và Reserved / Config không phải là vùng chương trình chính, nhưng cần được định nghĩa trong bootloader/application để đọc, ghi và bảo vệ đúng địa chỉ.

| Region            |     Origin | Length |
| ----------------- | ---------: | -----: |
| Metadata          | 0x08008000 |   8 KB |
| Reserved / Config | 0x0800A000 |   8 KB |

---

# Finite State Machine

Finite State Machine mô tả cách bootloader và application quản lý trạng thái firmware, OTA, confirm và rollback.

## Application Slot States

Mỗi application slot có thể thuộc một trong các trạng thái sau:

| Trạng thái | Ý nghĩa                                                         |
| ---------- | --------------------------------------------------------------- |
| Empty      | Slot chưa có firmware hợp lệ                                    |
| Valid      | Slot có firmware đã ghi và kiểm tra cơ bản hợp lệ               |
| Pending    | Firmware mới được cập nhật, đang chờ boot thử và confirm        |
| Confirmed  | Firmware đã chạy ổn định và được xác nhận                       |
| Invalid    | Firmware không hợp lệ hoặc đã boot thất bại quá số lần cho phép |

## Bootloader States

Bootloader có các trạng thái chính sau:

| Trạng thái                 | Mô tả                                                            |
| -------------------------- | ---------------------------------------------------------------- |
| Reset                      | MCU vừa reset và bắt đầu chạy bootloader                         |
| Load Metadata              | Bootloader đọc metadata từ Flash                                 |
| Validate Metadata          | Bootloader kiểm tra metadata bằng magic value, sequence và CRC   |
| Check OTA Request          | Bootloader kiểm tra có yêu cầu vào OTA mode hay không            |
| Select Slot                | Bootloader chọn application slot phù hợp để boot                 |
| Validate Application       | Bootloader kiểm tra firmware bằng CRC và kiểm tra địa chỉ hợp lệ |
| Boot Pending Application   | Bootloader boot firmware mới ở trạng thái pending                |
| Boot Confirmed Application | Bootloader boot firmware confirmed                               |
| Bootloader OTA Mode        | Bootloader nhận firmware mới qua UART                            |
| Mark Pending               | Bootloader đánh dấu firmware mới là pending                      |
| Rollback                   | Bootloader quay lại firmware confirmed trước đó                  |
| Safe Mode                  | Bootloader không tìm thấy firmware hợp lệ và chờ OTA             |

## Application Runtime OTA States

Khi OTA được thực hiện trong application, application có thể có các trạng thái sau:

| Trạng thái           | Mô tả                                                           |
| -------------------- | --------------------------------------------------------------- |
| App Running          | Application đang chạy bình thường                               |
| OTA Request Received | Application nhận được lệnh OTA qua UART                         |
| Select Inactive Slot | Application xác định slot không active để ghi firmware mới      |
| Erase Inactive Slot  | Application xóa vùng Flash của inactive slot                    |
| Receive Firmware     | Application nhận firmware qua UART                              |
| Write Firmware       | Application ghi firmware vào inactive slot                      |
| Verify Firmware      | Application kiểm tra CRC firmware đã ghi                        |
| Mark Pending         | Application cập nhật metadata, đánh dấu firmware mới là pending |
| Reboot To Bootloader | Application reset MCU để bootloader xử lý firmware mới          |
| OTA Failed           | OTA lỗi, application giữ nguyên firmware hiện tại               |

---

## Normal Boot Flow

Khi MCU reset, bootloader chạy đầu tiên.

Quy trình boot bình thường:

1. Bootloader đọc metadata.
2. Bootloader kiểm tra metadata hợp lệ.
3. Bootloader kiểm tra có OTA request flag hay không.
4. Nếu có OTA request flag, bootloader vào Bootloader UART OTA mode.
5. Nếu không có OTA request flag, bootloader kiểm tra có firmware pending hay không.
6. Nếu có firmware pending, bootloader xử lý Pending Firmware Flow.
7. Nếu không có firmware pending, bootloader chọn firmware confirmed.
8. Bootloader kiểm tra CRC của firmware confirmed.
9. Nếu firmware hợp lệ, bootloader jump vào application.
10. Application chạy blink LED và UART log.

Nếu không có firmware confirmed hợp lệ, bootloader vào safe mode hoặc OTA mode để chờ nạp firmware mới.

---

## Bootloader UART OTA Flow

Trong luồng này, bootloader là thành phần nhận firmware trực tiếp qua UART.

Quy trình:

1. Thiết bị reset vào bootloader.
2. Bootloader vào OTA mode.
3. Bootloader xác định inactive slot.
4. Host gửi firmware mới qua UART.
5. Bootloader kiểm tra kích thước firmware không vượt quá 232 KB.
6. Bootloader xóa Flash của inactive slot.
7. Bootloader ghi firmware mới vào inactive slot.
8. Bootloader tính CRC firmware đã ghi.
9. Nếu CRC đúng, bootloader cập nhật metadata.
10. Firmware mới được đánh dấu là pending.
11. Thiết bị reset.
12. Bootloader boot thử firmware pending.

Nếu Bootloader UART OTA thất bại, bootloader không đánh dấu firmware mới là pending. Firmware confirmed hiện tại vẫn được giữ nguyên.

---

## Runtime UART OTA Flow

Trong luồng này, application đang chạy là thành phần nhận firmware qua UART.

Quy trình:

1. Thiết bị đang chạy một application confirmed.
2. Host gửi OTA command qua UART.
3. Application chuyển sang trạng thái OTA receiving.
4. Application xác định inactive slot.
5. Host gửi firmware mới qua UART.
6. Application kiểm tra kích thước firmware không vượt quá 232 KB.
7. Application xóa Flash của inactive slot.
8. Application ghi firmware mới vào inactive slot.
9. Application tính CRC firmware đã ghi.
10. Nếu CRC đúng, application cập nhật metadata.
11. Firmware mới được đánh dấu là pending.
12. Application reset MCU.
13. Bootloader chạy sau reset.
14. Bootloader phát hiện firmware pending.
15. Bootloader kiểm tra CRC firmware pending.
16. Bootloader boot thử firmware mới.

Nếu Runtime UART OTA thất bại, application không cập nhật metadata sang pending. Thiết bị tiếp tục chạy firmware hiện tại.

---

## Pending Firmware Flow

Firmware mới sau OTA không được xem là confirmed ngay. Nó phải trải qua trạng thái pending.

Quy trình pending:

1. Bootloader phát hiện có slot pending.
2. Bootloader kiểm tra CRC của firmware pending.
3. Nếu CRC hợp lệ, bootloader tăng boot attempt counter.
4. Bootloader boot vào firmware pending.
5. Application pending khởi động.
6. Nếu application chạy ổn định, application cập nhật metadata để confirm firmware.
7. Sau khi confirm, firmware pending trở thành confirmed.
8. Boot attempt counter được reset.
9. Pending slot được xóa.
10. Slot này trở thành active slot.

Nếu application pending không confirm, lần reset tiếp theo bootloader tiếp tục xử lý theo số lần boot thử còn lại.

---

## Confirm Firmware Flow

Confirm firmware là bước application xác nhận rằng firmware hiện tại đã chạy ổn định.

Quy trình:

1. Bootloader boot vào firmware pending.
2. Application khởi tạo hệ thống.
3. Application kiểm tra các chức năng cơ bản đã hoạt động.
4. Application cập nhật metadata để xác nhận firmware hiện tại.
5. Slot hiện tại chuyển từ pending sang confirmed.
6. Slot hiện tại trở thành active slot.
7. Boot attempt counter được reset.
8. Application tiếp tục chạy bình thường.

Firmware chỉ được xem là ổn định sau khi confirm thành công.

---

## Rollback Flow

Rollback xảy ra khi firmware pending không xác nhận hoạt động ổn định sau số lần boot thử tối đa.

Quy trình rollback:

1. Bootloader phát hiện slot pending.
2. Bootloader kiểm tra boot attempt counter.
3. Nếu số lần boot thử chưa vượt giới hạn, bootloader tiếp tục boot firmware pending.
4. Nếu số lần boot thử vượt giới hạn, bootloader đánh dấu firmware pending là invalid.
5. Bootloader chọn previous slot hoặc confirmed slot gần nhất.
6. Bootloader kiểm tra CRC firmware rollback.
7. Nếu firmware rollback hợp lệ, bootloader boot lại firmware cũ.
8. Hệ thống tiếp tục hoạt động bằng firmware confirmed trước đó.

Rollback giúp thiết bị không bị brick khi firmware mới bị lỗi.

---

## Anti-brick Flow

Anti-brick được đảm bảo bằng cách luôn giữ lại ít nhất một firmware confirmed hợp lệ trong một application slot.

Các nguyên tắc anti-brick:

1. Firmware đang chạy không bị ghi đè trong quá trình OTA.
2. Firmware mới chỉ được ghi vào inactive slot.
3. Metadata chỉ được cập nhật sang pending sau khi firmware mới đã được ghi đầy đủ và CRC hợp lệ.
4. Firmware pending phải tự confirm sau khi chạy ổn định.
5. Nếu firmware pending không confirm, bootloader rollback về firmware confirmed.
6. Nếu OTA bị mất nguồn giữa chừng, metadata không chuyển sang pending và firmware cũ vẫn được boot.
7. Nếu application reset giữa quá trình OTA, bootloader bỏ qua firmware chưa được đánh dấu pending hợp lệ.
8. Nếu metadata mới bị lỗi, bootloader sử dụng metadata hợp lệ trước đó.
9. Nếu cả hai application đều không hợp lệ, bootloader vào safe mode để chờ OTA qua UART.

---

## Safe Mode

Safe mode là trạng thái bảo vệ cuối cùng của bootloader.

Bootloader vào safe mode khi:

- Không tìm thấy application confirmed hợp lệ.
- Cả hai application đều invalid hoặc empty.
- Metadata bị lỗi và không thể xác định firmware để boot.
- CRC của tất cả firmware đều không hợp lệ.
- Rollback slot không hợp lệ.

Trong safe mode, bootloader không jump vào application. Thay vào đó, bootloader chờ firmware mới qua UART OTA.

Safe mode giúp thiết bị vẫn có khả năng phục hồi bằng cách nạp lại firmware mới.

---

# State Transition Summary

## Application Slot State Transition

| Trạng thái hiện tại | Sự kiện                                     | Trạng thái tiếp theo           |
| ------------------- | ------------------------------------------- | ------------------------------ |
| Empty               | OTA ghi firmware thành công và CRC đúng     | Pending                        |
| Empty               | OTA thất bại hoặc CRC sai                   | Empty                          |
| Pending             | Application confirm thành công              | Confirmed                      |
| Pending             | Boot thất bại nhưng chưa vượt giới hạn      | Pending                        |
| Pending             | Boot thất bại quá giới hạn                  | Invalid                        |
| Confirmed           | Có OTA firmware mới vào slot còn lại        | Confirmed, slot mới là Pending |
| Invalid             | OTA ghi firmware mới thành công và CRC đúng | Pending                        |
| Any                 | CRC firmware sai                            | Invalid                        |
| Any                 | Không có firmware hợp lệ                    | Safe Mode                      |

## Runtime OTA State Transition

| Trạng thái hiện tại  | Sự kiện                      | Trạng thái tiếp theo |
| -------------------- | ---------------------------- | -------------------- |
| App Running          | Nhận OTA command qua UART    | OTA Request Received |
| OTA Request Received | Xác định được inactive slot  | Select Inactive Slot |
| Select Inactive Slot | Slot hợp lệ                  | Erase Inactive Slot  |
| Erase Inactive Slot  | Xóa Flash thành công         | Receive Firmware     |
| Receive Firmware     | Nhận data hợp lệ             | Write Firmware       |
| Write Firmware       | Ghi Flash thành công         | Verify Firmware      |
| Verify Firmware      | CRC đúng                     | Mark Pending         |
| Verify Firmware      | CRC sai                      | OTA Failed           |
| Mark Pending         | Metadata cập nhật thành công | Reboot To Bootloader |
| OTA Failed           | Hủy OTA                      | App Running          |

## Bootloader State Transition

| Trạng thái hiện tại      | Sự kiện                        | Trạng thái tiếp theo                      |
| ------------------------ | ------------------------------ | ----------------------------------------- |
| Reset                    | Bootloader bắt đầu chạy        | Load Metadata                             |
| Load Metadata            | Metadata hợp lệ                | Validate Metadata                         |
| Load Metadata            | Metadata không hợp lệ          | Safe Mode                                 |
| Validate Metadata        | Có OTA request flag            | Bootloader OTA Mode                       |
| Validate Metadata        | Có pending slot                | Validate Application                      |
| Validate Metadata        | Không có pending slot          | Select Slot                               |
| Select Slot              | Có confirmed slot hợp lệ       | Validate Application                      |
| Select Slot              | Không có confirmed slot hợp lệ | Safe Mode                                 |
| Validate Application     | Firmware hợp lệ và pending     | Boot Pending Application                  |
| Validate Application     | Firmware hợp lệ và confirmed   | Boot Confirmed Application                |
| Validate Application     | Firmware không hợp lệ          | Rollback hoặc Safe Mode                   |
| Boot Pending Application | Application confirm thành công | Confirmed                                 |
| Boot Pending Application | Application không confirm      | Pending hoặc Rollback                     |
| Rollback                 | Previous slot hợp lệ           | Boot Confirmed Application                |
| Rollback                 | Previous slot không hợp lệ     | Safe Mode                                 |
| Bootloader OTA Mode      | OTA thành công                 | Mark Pending                              |
| Bootloader OTA Mode      | OTA thất bại                   | Safe Mode hoặc Boot Confirmed Application |
| Mark Pending             | Metadata cập nhật thành công   | Reset                                     |

---

# Demo Scenarios

## Scenario 1: Boot bình thường

1. Application 1 đang ở trạng thái confirmed.
2. MCU reset.
3. Bootloader kiểm tra metadata.
4. Bootloader chọn Application 1.
5. Bootloader kiểm tra CRC Application 1.
6. Bootloader boot Application 1.
7. Application 1 blink LED chậm và log UART.

## Scenario 2: Runtime OTA thành công khi application đang chạy

1. Thiết bị đang chạy Application 1 confirmed.
2. Application 1 blink LED chậm và log UART.
3. Host gửi OTA command qua UART.
4. Application 1 chuyển sang trạng thái OTA receiving.
5. Application 1 xác định Application 2 là inactive slot.
6. Application 1 nhận firmware mới qua UART.
7. Application 1 ghi firmware mới vào Application 2.
8. Application 1 kiểm tra CRC Application 2.
9. CRC hợp lệ nên Application 1 đánh dấu Application 2 là pending.
10. Application 1 reset MCU.
11. Bootloader chạy sau reset.
12. Bootloader phát hiện Application 2 pending.
13. Bootloader kiểm tra CRC Application 2.
14. Bootloader boot Application 2.
15. Application 2 blink LED nhanh và log UART.
16. Application 2 confirm firmware.
17. Application 2 trở thành confirmed.

## Scenario 3: Bootloader UART OTA thành công

1. Thiết bị reset vào bootloader.
2. Bootloader vào OTA mode.
3. Bootloader xác định inactive slot.
4. Host gửi firmware mới qua UART.
5. Bootloader ghi firmware vào inactive slot.
6. Bootloader kiểm tra CRC firmware mới.
7. CRC hợp lệ nên bootloader đánh dấu firmware mới là pending.
8. MCU reset.
9. Bootloader boot thử firmware pending.
10. Application mới chạy và confirm.
11. Firmware mới trở thành confirmed.

## Scenario 4: Firmware mới bị lỗi và rollback

1. Thiết bị đang chạy Application 2 confirmed.
2. Host OTA firmware lỗi vào Application 1.
3. CRC firmware hợp lệ về mặt truyền dữ liệu.
4. Application 1 được đánh dấu là pending.
5. MCU reset.
6. Bootloader boot thử Application 1.
7. Application 1 lỗi, không confirm hoặc reset liên tục.
8. Bootloader tăng boot attempt counter sau mỗi lần thử.
9. Khi vượt quá số lần boot thử tối đa, bootloader đánh dấu Application 1 là invalid.
10. Bootloader rollback về Application 2.
11. Application 2 tiếp tục chạy bình thường.

## Scenario 5: Mất nguồn khi Runtime OTA

1. Thiết bị đang chạy Application 1 confirmed.
2. Host bắt đầu gửi firmware mới qua UART.
3. Application 1 đang ghi firmware vào Application 2.
4. Mất nguồn trước khi firmware được ghi xong hoặc trước khi metadata được cập nhật.
5. MCU reset.
6. Bootloader đọc metadata.
7. Vì Application 2 chưa được đánh dấu pending hợp lệ, bootloader bỏ qua Application 2.
8. Bootloader tiếp tục boot Application 1 confirmed.
9. Thiết bị không bị brick.

## Scenario 6: Mất nguồn khi Bootloader UART OTA

1. Thiết bị đang ở Bootloader UART OTA mode.
2. Host gửi firmware mới qua UART.
3. Bootloader đang ghi firmware vào inactive slot.
4. Mất nguồn trước khi firmware được ghi xong hoặc trước khi metadata được cập nhật.
5. MCU reset.
6. Bootloader đọc metadata.
7. Firmware mới chưa được đánh dấu pending hợp lệ.
8. Bootloader tiếp tục boot firmware confirmed hiện tại nếu có.
9. Nếu không có firmware confirmed hợp lệ, bootloader vào safe mode để chờ OTA lại.

## Scenario 7: Không có application hợp lệ

1. MCU reset.
2. Bootloader đọc metadata.
3. Bootloader không tìm thấy Application 1 hoặc Application 2 hợp lệ.
4. Bootloader không jump vào application.
5. Bootloader vào safe mode.
6. Bootloader chờ host nạp firmware mới qua UART.

---

# Design Notes

Thiết kế này ưu tiên tính đơn giản, dễ demo và dễ quan sát trên STM32 có Flash 512 KB.

Các quyết định thiết kế chính:

- Bootloader đặt ở đầu Flash để MCU chạy đầu tiên sau reset.
- Metadata tách riêng khỏi bootloader và application.
- Reserved / Config tách riêng khỏi metadata để lưu dữ liệu cấu hình hoặc dữ liệu mở rộng.
- Hai application slots có kích thước bằng nhau để hỗ trợ update luân phiên.
- Flash có 256 page, mỗi page 2 KB nên toàn bộ layout được align theo page.
- OTA chỉ ghi vào inactive slot.
- Active slot không bị ghi đè trong quá trình OTA.
- CRC được dùng để kiểm tra tính toàn vẹn firmware.
- Firmware mới phải qua trạng thái pending trước khi confirmed.
- Application phải confirm firmware mới sau khi chạy ổn định.
- Rollback dựa trên boot attempt counter.
- Safe mode cho phép khôi phục thiết bị nếu không còn firmware hợp lệ.
- Hệ thống hỗ trợ cả Bootloader UART OTA và Runtime UART OTA từ application.

Thiết kế này phù hợp cho demo bootloader với các chức năng CRC, rollback, anti-brick và OTA qua UART trên STM32 512 KB Flash.
