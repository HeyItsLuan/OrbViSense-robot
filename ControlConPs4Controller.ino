#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <Bluepad32.h>

ControllerPtr myControllers[BP32_MAX_GAMEPADS];

// =========================================================
// MOTOR DERECHO
// =========================================================

#define PWMA 13
#define AIN2 12
#define AIN1 14

// =========================================================
// MOTOR IZQUIERDO
// =========================================================

#define PWMB 25
#define BIN1 27
#define BIN2 26

// =========================================================
// CONFIG PASOS (D-PAD)
// =========================================================

#define STEP_SPEED    135
#define STEP_DURATION 100

// =========================================================
// CONFIG JOYSTICK
// =========================================================

#define JOYSTICK_MAX_SPEED 190

// =========================================================
// CONFIG RAMPA
// =========================================================

#define RAMP_RATE 200.0

// =========================================================
// BOTON BOOT Y LED
// =========================================================

#define BOOT_BUTTON 0
#define LED_PIN 2

// =========================================================
// WIFI
// =========================================================

#define WIFI_SSID     "LUAN"
#define WIFI_PASSWORD "MAYAMAYA"

// =========================================================
// ROS / WEBSOCKET
// =========================================================

#define ROS_IP   "10.42.0.1"
#define ROS_PORT 9090

#define ROS_PWM_TOPIC "/robot/pwm"

// =========================================================
// MODOS
// =========================================================

enum RobotMode
{
    MODE_CONTROLLER,
    MODE_ROS
};

RobotMode currentMode =
    MODE_CONTROLLER;

// =========================================================
// WEBSOCKET
// =========================================================

WebSocketsClient webSocket;

bool webSocketConnected = false;
bool webSocketStarted = false;

// =========================================================
// VELOCIDADES
// =========================================================

float currentLeftSpeed = 0;
float currentRightSpeed = 0;

unsigned long lastRampTime = 0;

// =========================================================
// LED
// =========================================================

unsigned long lastBlinkTime = 0;

bool ledState = false;

// =========================================================
// BOTON
// =========================================================

bool lastBootButtonState = HIGH;

// =========================================================
// CONTROL
// =========================================================

void onConnectedController(ControllerPtr ctl)
{
    Serial.println("Control conectado!");
    myControllers[0] = ctl;
}

void onDisconnectedController(ControllerPtr ctl)
{
    Serial.println("Control desconectado");
    myControllers[0] = nullptr;
}

// =========================================================
// MOTORES
// =========================================================

void setupMotors()
{
    pinMode(AIN1, OUTPUT);
    pinMode(AIN2, OUTPUT);

    pinMode(BIN1, OUTPUT);
    pinMode(BIN2, OUTPUT);

    // PWM canal 0
    ledcSetup(0, 1000, 8);
    ledcAttachPin(PWMA, 0);

    // PWM canal 1
    ledcSetup(1, 1000, 8);
    ledcAttachPin(PWMB, 1);

    ledcWrite(0, 0);
    ledcWrite(1, 0);
}

// =========================================================

void setMotorRight(int speed)
{
    speed = constrain(speed, -255, 255);

    if (speed > 0)
    {
        digitalWrite(AIN1, HIGH);
        digitalWrite(AIN2, LOW);

        ledcWrite(0, speed);
    }

    else if (speed < 0)
    {
        digitalWrite(AIN1, LOW);
        digitalWrite(AIN2, HIGH);

        ledcWrite(0, -speed);
    }

    else
    {
        ledcWrite(0, 0);

        digitalWrite(AIN1, LOW);
        digitalWrite(AIN2, LOW);
    }
}

// =========================================================

void setMotorLeft(int speed)
{
    speed = constrain(speed, -255, 255);

    if (speed > 0)
    {
        digitalWrite(BIN1, HIGH);
        digitalWrite(BIN2, LOW);

        ledcWrite(1, speed);
    }

    else if (speed < 0)
    {
        digitalWrite(BIN1, LOW);
        digitalWrite(BIN2, HIGH);

        ledcWrite(1, -speed);
    }

    else
    {
        ledcWrite(1, 0);

        digitalWrite(BIN1, LOW);
        digitalWrite(BIN2, LOW);
    }
}

// =========================================================

void stopMotors()
{
    setMotorLeft(0);
    setMotorRight(0);

    currentLeftSpeed = 0;
    currentRightSpeed = 0;
}

// =========================================================

void doStep(int leftSpeed, int rightSpeed)
{
    setMotorLeft(leftSpeed);
    setMotorRight(rightSpeed);

    delay(STEP_DURATION);

    stopMotors();
}

// =========================================================
// RAMPA
// =========================================================

float rampToTime(
    float current,
    float target,
    float ratePerSecond,
    float deltaSeconds
)
{
    float maxChange =
        ratePerSecond * deltaSeconds;

    if (current < target)
    {
        current += maxChange;

        if (current > target)
            current = target;
    }

    else if (current > target)
    {
        current -= maxChange;

        if (current < target)
            current = target;
    }

    return current;
}

// =========================================================
// WEBSOCKET
// =========================================================

void webSocketEvent(
    WStype_t type,
    uint8_t *payload,
    size_t length
)
{
    switch (type)
    {
        case WStype_DISCONNECTED:

            Serial.println(
                "[ROS] WebSocket desconectado"
            );

            webSocketConnected = false;

            break;


        case WStype_CONNECTED:

            Serial.println(
                "[ROS] WebSocket conectado"
            );

            webSocketConnected = true;

            // Suscribirse al unico topico PWM
            webSocket.sendTXT(
                "{\"op\":\"subscribe\",\"topic\":\"/robot/pwm\"}"
            );

            Serial.println(
                "[ROS] Suscrito a /robot/pwm"
            );

            break;


        case WStype_TEXT:
        {
            StaticJsonDocument<512> doc;

            DeserializationError error =
                deserializeJson(
                    doc,
                    payload,
                    length
                );

            if (error)
            {
                Serial.println(
                    "[ROS] Error al interpretar JSON"
                );

                return;
            }

            const char *topic =
                doc["topic"];

            if (topic == nullptr)
                return;

            if (
                strcmp(
                    topic,
                    ROS_PWM_TOPIC
                ) != 0
            )
            {
                return;
            }

            JsonObject msg =
                doc["msg"];

            if (msg.isNull())
            {
                Serial.println(
                    "[ROS] Mensaje sin campo msg"
                );

                return;
            }

            if (
                !msg["left"].is<int>() ||
                !msg["right"].is<int>()
            )
            {
                Serial.println(
                    "[ROS] PWM incompleto"
                );

                return;
            }

            int left =
                msg["left"];

            int right =
                msg["right"];

            left =
                constrain(
                    left,
                    -255,
                    255
                );

            right =
                constrain(
                    right,
                    -255,
                    255
                );

            Serial.print(
                "[ROS] LEFT="
            );

            Serial.print(left);

            Serial.print(
                " RIGHT="
            );

            Serial.println(right);

            // Aplicacion inmediata.
            setMotorLeft(left);
            setMotorRight(right);

            // Mantener el ultimo comando recibido.
            currentLeftSpeed =
                left;

            currentRightSpeed =
                right;

            break;
        }


        default:
            break;
    }
}

// =========================================================
// INICIAR MODO ROS
// =========================================================

void iniciarModoROS()
{
    Serial.println();
    Serial.println(
        "================================"
    );
    Serial.println(
        "[MODO] CAMBIANDO A ROS"
    );
    Serial.println(
        "================================"
    );

    // Seguridad: detener motores
    stopMotors();

    webSocketConnected = false;
    webSocketStarted = false;

    WiFi.disconnect(true);

    delay(100);

    WiFi.mode(WIFI_STA);

    Serial.print(
        "[ROS] Conectando a WiFi: "
    );

    Serial.println(
        WIFI_SSID
    );

    WiFi.begin(
        WIFI_SSID,
        WIFI_PASSWORD
    );
}

// =========================================================
// SALIR DE MODO ROS
// =========================================================

void detenerModoROS()
{
    Serial.println();
    Serial.println(
        "================================"
    );
    Serial.println(
        "[MODO] SALIENDO DE ROS"
    );
    Serial.println(
        "================================"
    );

    // Seguridad
    stopMotors();

    webSocketConnected = false;

    if (webSocketStarted)
    {
        webSocket.disconnect();
    }

    webSocketStarted = false;

    WiFi.disconnect(true);

    delay(100);
}

// =========================================================
// CAMBIO DE MODO
// =========================================================

void cambiarModo()
{
    // Seguridad inmediata
    stopMotors();

    if (
        currentMode ==
        MODE_CONTROLLER
    )
    {
        currentMode =
            MODE_ROS;

        iniciarModoROS();
    }
    else
    {
        detenerModoROS();

        currentMode =
            MODE_CONTROLLER;

        Serial.println(
            "[MODO] CONTROL BLUETOOTH"
        );
    }

    // Reiniciar estado del LED
    lastBlinkTime =
        millis();

    ledState = false;

    digitalWrite(
        LED_PIN,
        LOW
    );
}

// =========================================================
// BOTON BOOT
// =========================================================

void revisarBoot()
{
    bool buttonState =
        digitalRead(
            BOOT_BUTTON
        );

    // Detectar solamente el flanco
    // HIGH -> LOW.
    if (
        lastBootButtonState == HIGH &&
        buttonState == LOW
    )
    {
        Serial.println(
            "[BOTON] BOOT pulsado"
        );

        cambiarModo();
    }

    lastBootButtonState =
        buttonState;
}

// =========================================================
// LED DE ESTADO
// =========================================================

void actualizarLED()
{
    bool conectado = false;

    if (
        currentMode ==
        MODE_CONTROLLER
    )
    {
        conectado =
            myControllers[0] &&
            myControllers[0]->isConnected();
    }
    else
    {
        conectado =
            webSocketConnected;
    }

    // Conexion correcta:
    // LED apagado.
    if (conectado)
    {
        ledState = false;

        digitalWrite(
            LED_PIN,
            LOW
        );

        return;
    }

    // Sin conexion:
    // LED parpadeando.
    unsigned long now =
        millis();

    if (
        now - lastBlinkTime >=
        250
    )
    {
        lastBlinkTime =
            now;

        ledState =
            !ledState;

        digitalWrite(
            LED_PIN,
            ledState
        );
    }
}

// =========================================================
// PROCESAR ROS
// =========================================================

void procesarROS()
{
    // -----------------------------------------------------
    // Todavia no hay WiFi
    // -----------------------------------------------------

    if (
        WiFi.status() !=
        WL_CONNECTED
    )
    {
        webSocketConnected = false;

        return;
    }

    // -----------------------------------------------------
    // WiFi conectado
    // -----------------------------------------------------

    if (!webSocketStarted)
    {
        Serial.println(
            "[ROS] WiFi conectado"
        );

        Serial.print(
            "[ROS] IP ESP32: "
        );

        Serial.println(
            WiFi.localIP()
        );

        Serial.print(
            "[ROS] Conectando WebSocket a "
        );

        Serial.print(
            ROS_IP
        );

        Serial.print(
            ":"
        );

        Serial.println(
            ROS_PORT
        );

        webSocket.begin(
            ROS_IP,
            ROS_PORT,
            "/"
        );

        webSocket.onEvent(
            webSocketEvent
        );

        webSocket.setReconnectInterval(
            2000
        );

        webSocketStarted = true;
    }

    // Mantener WebSocket funcionando
    webSocket.loop();
}

// =========================================================
// SETUP
// =========================================================

void setup()
{
    Serial.begin(115200);

    // =====================================================
    // BOTON
    // =====================================================

    pinMode(
        BOOT_BUTTON,
        INPUT_PULLUP
    );

    // =====================================================
    // LED
    // =====================================================

    pinMode(
        LED_PIN,
        OUTPUT
    );

    digitalWrite(
        LED_PIN,
        LOW
    );

    // =====================================================
    // MOTORES
    // =====================================================

    setupMotors();

    // =====================================================
    // BLUEPAD32
    // =====================================================

    BP32.setup(
        &onConnectedController,
        &onDisconnectedController
    );

    BP32.forgetBluetoothKeys();

    lastRampTime =
        millis();

    Serial.println(
        "Esperando control..."
    );
}

// =========================================================
// LOOP
// =========================================================

void loop()
{
    // =====================================================
    // BOTON BOOT
    // =====================================================

    revisarBoot();

    // =====================================================
    // LED
    // =====================================================

    actualizarLED();

    // =====================================================
    // MODO ROS
    // =====================================================

    if (
        currentMode ==
        MODE_ROS
    )
    {
        procesarROS();

        delay(5);

        return;
    }

    // =====================================================
    // MODO CONTROL
    // =====================================================

    BP32.update();

    // Tiempo real transcurrido desde la ultima vez
    // que actualizamos la rampa
    unsigned long now = millis();

    float deltaSeconds =
        (now - lastRampTime) / 1000.0;

    lastRampTime = now;

    if (
        myControllers[0] &&
        myControllers[0]->isConnected()
    )
    {
        // =================================================
        // D-PAD: pasos fijos a baja velocidad
        // =================================================

        static uint8_t lastDpad = 0;

        uint8_t dpad =
            myControllers[0]->dpad();

        if (
            dpad != 0 &&
            dpad != lastDpad
        )
        {
            if (
                dpad & DPAD_UP
            )
            {
                Serial.println(
                    "Paso adelante"
                );

                doStep(
                    STEP_SPEED,
                    STEP_SPEED
                );
            }

            else if (
                dpad & DPAD_DOWN
            )
            {
                Serial.println(
                    "Paso atras"
                );

                doStep(
                    -STEP_SPEED,
                    -STEP_SPEED
                );
            }

            else if (
                dpad & DPAD_LEFT
            )
            {
                Serial.println(
                    "Giro izquierda"
                );

                doStep(
                    -(STEP_SPEED + 45),
                    STEP_SPEED + 45
                );
            }

            else if (
                dpad & DPAD_RIGHT
            )
            {
                Serial.println(
                    "Giro derecha"
                );

                doStep(
                    STEP_SPEED + 45,
                    -(STEP_SPEED + 45)
                );
            }

            lastRampTime =
                millis();
        }

        lastDpad =
            dpad;

        // Si se esta usando el D-pad,
        // no leemos el joystick este ciclo
        if (dpad != 0)
        {
            delay(10);

            return;
        }

        // =================================================
        // JOYSTICK DERECHO
        // =================================================

        int x =
            myControllers[0]->axisRX();

        int y =
            myControllers[0]->axisRY();

        // Convertir rango
        x =
            map(
                x,
                -512,
                512,
                -JOYSTICK_MAX_SPEED,
                JOYSTICK_MAX_SPEED
            );

        y =
            map(
                y,
                -512,
                512,
                -JOYSTICK_MAX_SPEED,
                JOYSTICK_MAX_SPEED
            );

        // Invertir adelante
        y = -y;

        // Deadzone
        if (abs(x) < 20)
            x = 0;

        if (abs(y) < 20)
            y = 0;

        // =================================================
        // Mezcla diferencial
        // =================================================

        int targetLeftSpeed =
            y + x;

        int targetRightSpeed =
            y - x;

        targetLeftSpeed =
            constrain(
                targetLeftSpeed,
                -JOYSTICK_MAX_SPEED,
                JOYSTICK_MAX_SPEED
            );

        targetRightSpeed =
            constrain(
                targetRightSpeed,
                -JOYSTICK_MAX_SPEED,
                JOYSTICK_MAX_SPEED
            );

        // =================================================
        // MOVIMIENTO INSTANTANEO DEL JOYSTICK
        // =================================================

        setMotorLeft(targetLeftSpeed);
        setMotorRight(targetRightSpeed);

        currentLeftSpeed = targetLeftSpeed;
        currentRightSpeed = targetRightSpeed;
        // =================================================
        // DEBUG
        // =================================================

        Serial.print("X: ");
        Serial.print(x);

        Serial.print(" Y: ");
        Serial.print(y);

        Serial.print(" L: ");
        Serial.print(
            (int)currentLeftSpeed
        );

        Serial.print(" R: ");
        Serial.println(
            (int)currentRightSpeed
        );
    }

    delay(10);
}