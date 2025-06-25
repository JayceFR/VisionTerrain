import cv2
import struct 
import mediapipe as mp
import socket
import math

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
WSL_IP = "172.26.95.134" 
PORT = 5005

KEYS = [1, 2, 33, 263, 70, 300, 13, 14, 152]

mp_face = mp.solutions.face_mesh
cap = cv2.VideoCapture(0)

with mp_face.FaceMesh(refine_landmarks=True) as face_mesh:
  while cap.isOpened():
    ret, frame = cap.read()
    if not ret: break

    rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    result = face_mesh.process(rgb)
    if result.multi_face_landmarks:
        face = result.multi_face_landmarks[0].landmark
        left_eye = face[33]
        right_eye = face[263]
        nose = face[1]

        dx = right_eye.x - left_eye.x
        dy = right_eye.y - left_eye.y
        yaw = math.atan2(dx, dy) 
        pitch = -(nose.y - 0.5) * 2

        floats = []
        for i in KEYS:
           pt = face[i]
           floats += [pt.x, pt.y, pt.z]
        
        floats += [yaw, pitch]

        # msg = f"{yaw:.4f},{pitch:.4f}"
        print(yaw, pitch)

        data = struct.pack(f'{len(floats)}f', *floats)
        sock.sendto(data, (WSL_IP, PORT))
