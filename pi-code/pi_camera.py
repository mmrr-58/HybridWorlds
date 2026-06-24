from picamera2 import Picamera2
from pyzbar.pyzbar import decode

def scan_year():
    """Block until a QR code is scanned, then return its value as an int (the year)."""
    picam = Picamera2()
    picam.configure(picam.create_preview_configuration(main={"format": "RGB888", "size": (640, 480)}))
    picam.start()
    try:
        while True:
            img = picam.capture_array()
            for code in decode(img):
                year = int(code.data.decode("utf-8"))
                print(f"QR scanned: year {year}")
                return year
    finally:
        picam.close()
