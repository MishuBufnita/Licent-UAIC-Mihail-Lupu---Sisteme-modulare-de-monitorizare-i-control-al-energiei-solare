from flask import Flask, request, jsonify
from influxdb_client import InfluxDBClient, Point, WritePrecision
import socket
from influxdb_client.client.write_api import SYNCHRONOUS

app = Flask(__name__)

# Configurare InfluxDB
token = "exFnozOTc43yMcmjcVpLyZtKtSYuusonwyN6Di2keBsnSlx6CUGDOSfz7Ln4zTCqRkRoGYwXgp-GnRUlsrvqDQ=="
org = "Licență"
url = "http://localhost:8086"
bucket = "Panouri"

# Crearea clientului InfluxDB
client = InfluxDBClient(url=url, token=token, org=org)
write_api = client.write_api(write_options=SYNCHRONOUS)

# Crearea rutei pentru primirea datelor
@app.route('/data', methods=['POST'])
def receive_data():
    data = request.json

    # Extragem datele din request
    panel_id = data.get('panel_id')
    voltage_in = data.get('voltage_in')
    current_in = data.get('current_in')
    voltage_out = data.get('voltage_out')
    current_out = data.get('current_out')
    temperature = data.get('temperature')
    ambient_temp = data.get('ambient_temperature')
    humidity = data.get('humidity')
    light = data.get('light')
    uv = data.get('uv')

    # Inserarea datelor într-un format acceptat de InfluxDB
    point = (
        Point("sensor_data")
        .tag("panel_id", panel_id)
        .field("voltage_in", float(voltage_in))
        .field("current_in", float(current_in))
        .field("voltage_out", float(voltage_out))
        .field("current_out", float(current_out))
        .field("temperature", float(temperature))
        .field("ambient_temperature", float(ambient_temp))
        .field("humidity", float(humidity))
        .field("light", float(light))
        .field("uv", float(uv))
    )

    try:
        # Inserăm datele în InfluxDB
        write_api.write(bucket=bucket, org=org, record=point)
        return jsonify({"status": "success", "message": "Data inserted successfully"}), 200
    except Exception as e:
        return jsonify({"status": "error", "message": str(e)}), 500

if __name__ == '__main__':
    host = '0.0.0.0'
    port = 5000
    ip_address = socket.gethostbyname(socket.gethostname())
    print(f"Url server: http://{ip_address}:{port}")
    app.run(host=host, port=port, debug=True)
