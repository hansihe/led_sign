import subprocess
import paho.mqtt.client as mqtt
import time

jhadhsiu

def on_connect(client, userdata, flags, rc):
    print("connected")

client = mqtt.Client(client_id="led_mapper")
client.username_pw_set("hansihe_dev", "CYR3NU2JGnV9ZFsa")
client.on_connect = on_connect
client.connect("mqtt.hackheim", 1883, 60)

client.loop_start()
        
print("starting map")

for i in range(8):
    for k in range(60):
        print("Mapping {}_{}".format(i, k))
        p = client.publish("hackheim/bigsign/control", "t {} {} {}.0 {}.0 {}.0 d".format(i, k, 1, 1, 1))
        p.wait_for_publish()

        ret = subprocess.run(["curl", "http://10.20.30.221:8080/photo.jpg", "--output", "out/{}_{}.jpg".format(i, k)])
        assert(ret.returncode == 0)

print("Done")
