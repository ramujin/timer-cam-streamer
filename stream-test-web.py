
# import cv2

# def main():
#   stream_url = 'http://192.168.4.1:81/stream'
#   cam = cv2.VideoCapture(stream_url)

#   if cam.isOpened() is False:
#     raise Exception('Failed to grab video stream.')

#   success, frame = cam.read()
#   if not success or frame is None:
#     raise Exception('Failed to grab video stream.')

#   cv2.namedWindow("Stream", cv2.WINDOW_AUTOSIZE)

#   while True:
#     success, frame = cam.read()
#     if success:
#       cv2.imshow("Stream", frame)
#       key = cv2.waitKey(1)
#       if key == ord('q'):
#         break

#   cam.release()
#   cv2.destroyAllWindows()

# if __name__ == "__main__":
#   main()


import cv2
import numpy as np
import requests
from io import BytesIO
import time

def get_frame(stream_url):

  response = requests.get(stream_url, stream=True)

  image = None
  for chunk in response.iter_content(chunk_size=100000):
    if len(chunk) > 100:
      try:
        data = BytesIO(chunk)
        image = cv2.imdecode(np.frombuffer(data.read(), np.uint8), 1)
        break
      except Exception as e:
        print(e)
        continue

  return image

def main():
  cv2.namedWindow("Stream", cv2.WINDOW_AUTOSIZE)
  stream_url = "http://192.168.4.1:81/stream" # ESP-CAM

  fps_time = 0
  fps_counter = 0
  fps = 0
  fps_count = 100

  while True:
    start_time = time.time()
    image = get_frame(stream_url)
    end_time = time.time()
    # elapsed = end_time - start_time

    fps_counter += 1
    if fps_counter == fps_count:
      fps_counter = 0
      fps = fps_count / (end_time - fps_time)
      fps_time = end_time
    # print(f"Elapsed: {elapsed:0.2f} FPS: {fps:0.2f}")

    cv2.putText(image, f"FPS: {fps:0.2f}", org=(10,10), fontFace=cv2.FONT_HERSHEY_SIMPLEX, fontScale=0.5, color=(0,0,255), thickness=2)
    cv2.imshow("Stream", image)

    key = cv2.waitKey(1)
    if key == ord('q'):
      cv2.destroyAllWindows()
      exit()

if __name__ == "__main__":
  main()

