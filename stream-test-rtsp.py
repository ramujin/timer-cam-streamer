import cv2
import time

import os

# TODO: https://github.com/Ouroboros1337/python_rtsp_client/blob/main/rtsp_client.py

def main():
  cv2.namedWindow("Stream", cv2.WINDOW_AUTOSIZE)
  # os.environ['OPENCV_FFMPEG_CAPTURE_OPTIONS'] = 'rtsp_transport;udp'  # TODO: https://forum.opencv.org/t/how-to-add-ffmpeg-options-to-videocapture/8972/13
  # os.environ['OPENCV_FFMPEG_CAPTURE_OPTIONS'] = ''
  # cam = cv2.VideoCapture("rtsp://192.168.4.1:8554/mjpeg/1", cv2.CAP_FFMPEG)
  cam = cv2.VideoCapture("rtsp://192.168.4.1:8554/mjpeg/1")

  # https://github.com/geeksville/Micro-RTSP

  fps_time = 0
  fps_counter = 0
  fps = 0
  fps_count = 100

  while True:
    result, image = cam.read()
    if result is None:
      continue

    now = time.time()
    fps_counter += 1
    if fps_counter == fps_count:
      fps_counter = 0
      fps = fps_count / (now - fps_time)
      fps_time = now

    cv2.putText(image, f"FPS: {fps:0.2f}", org=(10,10), fontFace=cv2.FONT_HERSHEY_SIMPLEX, fontScale=0.5, color=(0,0,255), thickness=2)
    cv2.imshow("Stream", image)

    key = cv2.waitKey(1)
    if key == ord('q'):
      cv2.destroyAllWindows()
      cam.release()
      exit()

if __name__ == "__main__":
  main()
