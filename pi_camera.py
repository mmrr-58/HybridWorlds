import cv2
from picamera2 import Picamera2
from pyzbar.pyzbar import decode

picam = Picamera2()
picam.configure(picam.create_preview_configuration(main={"format": "RGB888", "size": (640, 480)}))
picam.start()

while True:
    img = picam.capture_array()

    for code in decode(img):
        data = code.data.decode("utf-8")
        x, y, w, h = code.rect
        cv2.rectangle(img, (x, y), (x + w, y + h), color=(255, 0, 255), thickness=2)
        cv2.putText(img, data, (x, y - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)
        print("data found:", data)

    cv2.imshow("code detector", img)
    if cv2.waitKey(1) == ord("q"):
        break

picam.close()
cv2.destroyAllWindows()