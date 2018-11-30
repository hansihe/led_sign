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

i = 0
while True:
    buf = bytearray()
    buf.append(10)

    i += randrange(-50, 100)

    for y in range(6):
        for x in range(20):
            buf.append(int((i + x*10 + y*15) / 360.0 * 255.0) % 255)
            buf.append(255)
            buf.append(200)

    p = client.publish("hackheim/bigsign/control", buf)
    p.wait_for_publish()
    time.sleep(1)
