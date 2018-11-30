import paho.mqtt.client as mqtt
from random import randrange
import time

def on_connect(client, userdata, flags, rc):
    print("connected")

client = mqtt.Client(client_id=credentals.CLIENT_ID)
client.username_pw_set(credentials.USERNAME, credentails.PASSWORD)
client.on_connect = on_connect
client.connect(credentials.HOSTNAME, credentials.PORT, 60)

client.loop_start()

#d = 1
#i = 0
#while True:
#    buf = bytearray()
#    buf.append(10)
#
#    i += d
#    if i < 0:
#        i = 1
#        d = 1
#    if i >= 20:
#        i = 18
#        d = -1
#
#    for y in range(6):
#        for x in range(20):
#            buf.append(0)
#            buf.append(0)
#            buf.append(50 if x == i else 0)
#
#    p = client.publish("hackheim/bigsign/control", buf)
#    p.wait_for_publish()
#    time.sleep(1)

i = 0
while True:
    buf = bytearray()
    buf.append(10)

    i += 50

    for y in range(6):
        for x in range(20):
            #buf.append((i + x*5 + y*10) % 255)
            # 0: Red
            # 2: Green
            # 4: Blue
            buf.append(int((i + x*10 + y*20) / 360.0 * 255.0) % 255)
            buf.append(255)
            buf.append(200)

    p = client.publish("hackheim/bigsign/control", buf)
    p.wait_for_publish()
    time.sleep(1)

#i = 0
#while True:
#    buf = bytearray()
#    buf.append(10)
#
#    sx = int(i % 20)
#    sy = int((i / 20) % 6)
#    i += 1
#
#    print(sx, sy)
#
#    for y in range(6):
#        for x in range(20):
#            #buf.append(randrange(150))
#            #buf.append(randrange(150))
#            #buf.append(randrange(150))
#            if x == sx and y == sy:
#                buf.append(i % 256)
#                buf.append(255)
#                buf.append(255)
#            else:
#                buf.append(i % 256)
#                buf.append(255)
#                buf.append(0)
#
#    p = client.publish("hackheim/bigsign/control", buf)
#    p.wait_for_publish()
#    print("posted", i)
#
#    time.sleep(0.2)
