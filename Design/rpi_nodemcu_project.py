### 필요한 라이브러리를 import 한다
from flask import Flask, render_template
from flask_mqtt import Mqtt
import json

app = Flask(__name__)


### MQTT 연결관련된 변수들이다
# app.config['MQTT_BROKER_URL'] = '203.252.106.154'
app.config['MQTT_BROKER_URL'] = 'sweetdream.iptime.org'
app.config['MQTT_BROKER_PORT'] = 1883
app.config['MQTT_USERNAME'] = 'iot'
app.config['MQTT_PASSWORD'] = 'csee1414'


mqtt = Mqtt(app)


### MQTT 관련 topic들이다
pub_topic = 'iot/{}'
sub_topic_dht22 = 'iot/{}/dht22'
sub_topic_cds = 'iot/{}/cds'

### 프로젝트에서 사용하는 학번과 NodeMCU에서 넘겨받은 dht22와 cds값을 저장한다
student_id = '21900050 21900780'.split()
dht22_json = {"21900050":'', "21900780":''}
cds_json = {"21900050":'', "21900780":''}


### NodeMCU에 명령을 내리기 위한 URL 이다
### <sid>와 <cmd>에 맞는 topic을 NodeMCU에 전송한다
@app.route('/iot/<sid>/<cmd>')
def index(sid, cmd):

    if cmd in 'led ledon ledoff dht22, cds, usbled usbledon usbledoff'.split():
        mqtt.publish(pub_topic.format(sid), cmd)
    
    # retrun default page
    return default_page("no error")



### 화면상의 온도, 습도, 조도 값을 업데이트 하기 위해 존재하는 API URL이다
### JSON값을 반환한다
@app.route('/api/update/<sid>')
def update_api(sid):
    try:
        dht22_data = json.loads(dht22_json[sid])
        cds_data = json.loads(cds_json[sid])
        dht22_data.update(cds_data)
        return json.dumps(dht22_data)
    except:
        dummy = {
            'dht22_t': '-',
            'dht22_h': '-',
            'cds_value': '-'
        }
        return json.dumps(dummy)


### Route all unhandled routes 
### 정의되지 않은 모든 url path에 대해 기본 페이지를 반환한다
@app.errorhandler(404)
def default_page(e):
    try:
        dht22_data = json.loads(dht22_json["21900050"])  # '{"dht22_t":12, "dht22_h":19}'
        t = dht22_data['dht22_t'] # 12
        h = dht22_data['dht22_h'] # 19
        cds_data = json.loads(cds_json["21900050"])      # '{"cds_value":18}'
        c = cds_data['cds_value']
    except:
        t, h, c = '-', '-', '-'
    return render_template('index.html', tttt=t, hhhh=h, cccc=c)


### When mqtt is connected, subscribe to folowing topics
###     MQTT가 브로커와 연결되었을 때 student_id 변수 안에 들어있는 학번들에 대해
###     dht22와 cds topic을 구독한다
@mqtt.on_connect()
def handle_connect(client, userdata, flags, rc):
    for id in student_id:
        mqtt.subscribe(sub_topic_dht22.format(id))
        mqtt.subscribe(sub_topic_cds.format(id))


### When mqtt receives message from subscribed topic
### 구독한 topic으로부터 메세지를 받았을 때 아래 함수가 실행된다
###    dictionary 형태의 global 변수인 dht22_json과 cds_json의 값을 직접 수정한다
###    student_id 변수에 저장되어 있는 학번과 관련된 topic만 저장한다
###    받은 topic과 메세지를 console에 출력한다
@mqtt.on_message()
def handle_mqtt_message(client, userdata, message):
    global dht22_json
    global cds_json
  
    topic = message.topic
    payload = message.payload.decode('utf-8')

    for id in student_id:
        if id in topic:
            if 'dht22' in topic:
                dht22_json[id] = payload
            elif 'cds' in topic:
                cds_json[id] = payload
    print("@@ Topic: " + topic + "  message: " + payload)



### 이 프로그램이 메인으로 실행되었을 조건이 True가 된다 
###     flask 앱을 실행한다
if __name__ == '__main__':
  app.run(host='0.0.0.0', port=5000, debug=True)