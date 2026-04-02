# ☀️ Pyrometer Temperature Monitoring
oleh : Husni dan Revan

Project ini digunakan untuk membaca data dari **pyrometer, PZEM DC, dan ds1303** menggunakan Arduino, kemudian menyimpan data untuk analisis.

## 📌 Fitur
- Membaca suhu secara real-time
- Membaca nilai radiasi secara real-time
- Membaca keluaran dari solar cell
- Simpan pengukuran per 1 menit di sd card
- Support komunikasi:
  - I2C (LCD I2C)
  - Modbus (industrial pyrometer, PZEM DC)

## 🧰 Requirements

Install dependency berikut:
- modbusmaster by doc Walker ver 1.0.0

Setelah diinstall buka C:/document/Arduino/Libraries/Modbusmaster lalu cari file modbusmaster.h dan ganti nilai parameter ini:
```modbusmaster.h
static const uint16_t ku16MBResponseTimeout          = 200; ///< Modbus timeout [milliseconds] 2000
```

- RTCLib by Adafruit ver 2.0.0



| Parameter     | Nilai       | Keterangan              |
|--------------|------------|------------------------|
| Sensor       | MLX90614   | Infrared temperature   |
| Interface    | I2C        | Address 0x5A           |
| Sampling     | 1 Hz       | 1 detik sekali         |


# Software Calibrator RTC
