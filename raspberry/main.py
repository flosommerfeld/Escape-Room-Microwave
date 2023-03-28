import RPi.GPIO as GPIO
from time import sleep
import serial, time, struct, threading
import paho.mqtt.client as mqtt

"""
WIFI:
    SSID = gameartifacts
    Password = blablabla
    Gateway = 192.168.22.1
"""

# MQTT config
MQTT_BROKER = "192.168.22.2"
MQTT_PORT = 1883
MQTT_KEEP_ALIVE = 60
MQTT_CLIENT_NAME = "microwave"

# MQTT topics
RESET_TOPIC = "/artifacts/microwave/reset"
SOLVE_TOPIC = "/artifacts/microwave/solve"

# Set up BCM GPIO numbering
GPIO.setmode(GPIO.BCM)

GPIO_SETTLE_TIME = 0.5 # in seconds
GPIO_PASSWORD_ORDER = [2, 27, 11, 19]
GPIO_PASSWORD_CURRENT_INDEX = 0

# GPIO pins of the touch sensors
touch_sensor_pins = [2,3,4,17,27,22,10,9,11,5,6,13,19,26,21,20,16,12,7,8,25,24]
TOUCH_SENSOR_DEBOUNCING_TIME = 0.5 # in seconds

correct_note_order = "E5F5G5G5F5E5"
input_note_order = [] # variable for storing the pressed note order

touch_sensor_piezo_notes_mapping = {
    "E5": [2, 6],
    "F5": [19, 7, 9],
    "G5": [16, 13, 10, 17, 21, 25, 20, 3],
    "F7": [11, 5, 12],
    "C3": [4, 26, 24, 22],
    "D6": [8, 27],   
}

# Reverse keys and values of the mapping so that getting the key of a value can be done easily
reversed_mapping = {pin: k for k, v in touch_sensor_piezo_notes_mapping.items() for pin in v}

# Setup all touch sensors as GPIO inputs
def setup_gpio_inputs(pins: list[int]):
    # Iterate over all pins and set them as inputs
    for pin in touch_sensor_pins:
        GPIO.setup(pin, GPIO.IN, pull_up_down=GPIO.PUD_UP)
    # Wait for GPIOs to settle
    sleep(GPIO_SETTLE_TIME)


# Serial connection with Arduino
ARDUINO_PORT = "/dev/ttyACM0"
BAUDRATE = 9600

# Codes for the arduino
OPEN_RELAY_CODE = 100
RESET_CODE = 200

# Callback for when the client receives a CONNACK response from the broker.
def on_connect(client, userdata, flags, result_code):
    print("Connected to MQTT broker with result code " + str(result_code))
    client.subscribe(RESET_TOPIC)
 
# Callback for when a PUBLISH message is received from the broker
def on_message(client, userdata, msg):
    print(msg.topic + " " + str(msg.payload))
    if msg.topic == RESET_TOPIC:
        ser.write(struct.pack("B", RESET_CODE))
        client.publish(SOLVE_TOPIC, "false")
    elif msg.topic == SOLVE_TOPIC:
        pass

# Callback for when the client publishes a message to the broker
def on_publish(client,userdata,result):
    print("Data published to MQTT broker")


def setup_mqtt():
    global client
    client = mqtt.Client(MQTT_CLIENT_NAME)
    client.on_connect = on_connect
    client.on_message = on_message
    client.on_publish = on_publish
    client.connect(MQTT_BROKER, MQTT_PORT, MQTT_KEEP_ALIVE)
    
    # Run MQTT loop in a separate thread
    def loop():
        client.loop_forever()
    threading.Thread(target=loop, daemon=True).start()
    
def setup_serial(port: str, baudrate: int) -> serial.Serial:
    global ser
    ser = serial.Serial(port, baudrate)

def get_active_touch_sensor(touch_sensor_pins: list[int]) -> int:
    for pin in touch_sensor_pins:
        if GPIO.input(pin) == GPIO.HIGH:
            return pin

def send_gpio_pin_number_to_serial(pin: int, ser: serial.Serial):
    # Send the active GPIO pin number to the Arduino
    ser.write(struct.pack("B",pin))

def check_password() -> bool:
    return "".join(input_note_order).endswith(correct_note_order)

def handle_input():
    try:  
        while True:
            # Read active touch sensor
            active_touch_sensor = get_active_touch_sensor(touch_sensor_pins)
            # Send active touch sensor pin number to the arduino
            if active_touch_sensor is not None:
                note = reversed_mapping[active_touch_sensor]
                print(note)
                input_note_order.append(note)
                send_gpio_pin_number_to_serial(active_touch_sensor, ser)
            if check_password():
                # Reset input
                input_note_order.clear()
                # Send the "correct password code" to the arduino
                ser.write(struct.pack("B", OPEN_RELAY_CODE))
                # Send message to MQTT topic
                client.publish(SOLVE_TOPIC, "true")
                
            # Delay for debouncing the touch sensor
            sleep(TOUCH_SENSOR_DEBOUNCING_TIME)
    finally:
        GPIO.cleanup()


def main():
    ser = setup_serial(ARDUINO_PORT, BAUDRATE)
    setup_mqtt()
    setup_gpio_inputs(touch_sensor_pins)
    handle_input()

if __name__ == "__main__":
    main()
