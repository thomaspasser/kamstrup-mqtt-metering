from collections import namedtuple
import paho.mqtt.client as mqtt
import json

from Crypto.Cipher import AES # Library: PyCryptodome
import gurux_dlms
from bs4 import BeautifulSoup

## Settings
mqtt_username = ''
mqtt_password = ''
mqtt_broker_ip = ''
mqtt_receive_topic = 'outTopic'
mqtt_send_topic = 'power/meter'
encryption_key = bytes.fromhex('') # gkp60
authentication_key = bytes.fromhex('') # gkp61

RegisterDef = namedtuple('RegisterDef', ['name', 'unit', 'scaler'])

### Registers defined in the DLSM-COSEM document         
registers = {'1.1.0.2.129.255': RegisterDef('OBIS list version identifier', '', None),
             '1.1.1.8.0.255': RegisterDef('Active energy A14', 'Wh', 0),
             '1.1.2.8.0.255': RegisterDef('Active energy A23', 'Wh', 0),
             '1.1.3.8.0.255': RegisterDef('Reactive energy R12', 'varh', 0),
             '1.1.4.8.0.255': RegisterDef('Reactive energy R34', 'varh', 0),
             '1.1.0.0.1.255': RegisterDef('Meter number 1', '', None),
             '1.1.1.7.0.255': RegisterDef('Actual power P14', 'W', 0),
             '1.1.2.7.0.255': RegisterDef('Actual power P23', 'W', 0),
             '1.1.3.7.0.255': RegisterDef('Actual power Q12', 'var', 0),
             '1.1.4.7.0.255': RegisterDef('Actual power Q34', 'var', 0),
             '0.1.1.0.0.255': RegisterDef('Real-time clock', '', None),
             '1.1.32.7.0.255': RegisterDef('RMS voltage of phase L1', 'V', 0),
             '1.1.52.7.0.255': RegisterDef('RMS voltage of phase L2', 'V', 0),
             '1.1.72.7.0.255': RegisterDef('RMS voltage of phase L3', 'V', 0),
             '1.1.31.7.0.255': RegisterDef('RMS current of phase L1', 'A', -2),
             '1.1.51.7.0.255': RegisterDef('RMS current of phase L2', 'A', -2),
             '1.1.71.7.0.255': RegisterDef('RMS current of phase L3', 'A', -2),
             '1.1.21.7.0.255': RegisterDef('Actual power P14 of phase L1', 'W', 0),
             '1.1.41.7.0.255': RegisterDef('Actual power P14 of phase L2', 'W', 0),
             '1.1.61.7.0.255': RegisterDef('Actual power P14 of phase L3', 'W', 0),
             '1.1.33.7.0.255': RegisterDef('Power factor of phase L1 -', '', None),
             '1.1.53.7.0.255': RegisterDef('Power factor of phase L2 -', '', None),
             '1.1.73.7.0.255': RegisterDef('Power factor of phase L3 -', '', None),
             '1.1.13.7.0.255': RegisterDef('Power factor Total','', None),
             '1.1.22.7.0.255': RegisterDef('Active power P23 of phase L1', 'W', 0),
             '1.1.42.7.0.255': RegisterDef('Active power P23 of phase L2', 'W', 0),
             '1.1.62.7.0.255': RegisterDef('Active power P23 of phase L3', 'W', 0),
             '1.1.22.8.0.255': RegisterDef('Active energy A23 of phase L1', 'Wh', 0),
             '1.1.42.8.0.255': RegisterDef('Active energy A23 of phase L2', 'Wh', 0),
             '1.1.62.8.0.255': RegisterDef('Active energy A23 of phase L3', 'Wh', 0),
             '1.1.21.8.0.255': RegisterDef('Active energy A14 of phase L1', 'Wh', 0),
             '1.1.41.8.0.255': RegisterDef('Active Energy A14 of phase L2', 'Wh', 0),
             '1.1.61.8.0.255': RegisterDef('Active Energy A14 of phase L3', 'Wh', 0)}

def hexToLogicalName(value):
    return '.'.join((str(int(value[k:k+2],16)) for k in range(0, len(value), 2)))

def on_message(client, userdata, msg):
    cipher_text = msg.payload
    system_title = cipher_text[2:2+8]

    initialization_vector = system_title + cipher_text[14:14+4]

    additional_authenticated_data = cipher_text[13:13+1] + authentication_key

    authentication_tag = cipher_text[-12:]

    cipher = AES.new(encryption_key,
                     AES.MODE_GCM,
                     nonce=initialization_vector,
                     mac_len=len(authentication_tag))

    cipher.update(additional_authenticated_data)

    plaintext = cipher.decrypt_and_verify(cipher_text[18:len(cipher_text)-12], authentication_tag)

    t = gurux_dlms.GXDLMSTranslator()
    xml = t.pduToXml(plaintext)

    soup = BeautifulSoup(xml, 'xml')
    structure = soup.NotificationBody.DataValue.Structure

    children = list(structure.children)

    res = {}
    for i in range(3, len(c), 4):
        logicalName = hexToLogicalName(children[i]['Value'])

        registerDef = registers[logicalName]
        if registerDef.scaler is None:
            val = int(children[i+2]['Value'], 16)
        else:
            val = int(children[i+2]['Value'], 16) * 10**registerDef.scaler
        res[registerDef.name.replace(" ", "_")] = val

    client.publish(mqtt_send_topic, payload=json.dumps(res))

client = mqtt.Client()
client.on_message = on_message

client.username_pw_set(mqtt_username, password=mqtt_password)
client.connect(mqtt_broker_ip, 1883, 60)
client.subscribe(mqtt_receive_topic)

client.loop_forever()
