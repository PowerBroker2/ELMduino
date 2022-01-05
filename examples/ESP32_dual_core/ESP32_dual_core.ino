#include <BluetoothSerial.h>
#include <ELMduino.h>
#include <atomic>

const uint8_t led = 2;

ELM327 obd;
BluetoothSerial SerialBT;
TaskHandle_t rpm_task;
TaskHandle_t led_task;

std::atomic<short> rpm{0};

void setup(void)
{
    Serial.begin(115200);

    connect_to_obd();

    pinMode(led, OUTPUT);

    // Do blocking OBD call on one core
    xTaskCreatePinnedToCore(
        get_rpm_task, // Task function.
        "Task1",      // Name of task.
        10000,        // Stack size of task
        NULL,         // Parameter of the task
        0,            // Priority of the task
        &rpm_task,    // Task handle to keep track of created task
        0);           // Pin task to core 0

    // Do timing specific work on another core
    xTaskCreatePinnedToCore(
        flash_led_task, // Task function.
        "Task2",        // Name of task.
        10000,          // Stack size of task
        NULL,           // Parameter of the task
        0,              // Priority of the task
        &led_task,      // Task handle to keep track of created task
        1);             // Pin task to core 1

    delay(500);
}

void loop()
{
    vTaskDelete(NULL);
}

void flash_led_task(void* parameters)
{
    static unsigned long current_time{0};
    static unsigned long previous_flash_time{0};
    static const int flash_interval{75};
    static bool is_on = false;

    for (;;)
    {
        current_time = millis();

        if (rpm > 2500)
        {
            if (current_time - previous_flash_time >= flash_interval)
            {
                if (is_on)
                {
                    digitalWrite(led, LOW);
                }
                else
                {
                    digitalWrite(led, HIGH);
                }

                previous_flash_time = current_time;
                is_on = !is_on;
            }
        }
        else
        {
            is_on = false;
            digitalWrite(led, LOW);
        }

        // Sadly you need this or you will starve the IDLE task
        delay(1);
    }
}

void get_rpm_task(void* parameters)
{
    for (;;)
    {
        read_rpm();
        // Sadly you need this or you will starve the IDLE task
        delay(1);
    }
}

void connect_to_obd()
{
    SerialBT.begin("led_flasher", true);

    bool connected = SerialBT.connect("OBDII");

    if(connected)
    {
        Serial.println("Connected Succesfully!");
    }
    else
    {
        while(!SerialBT.connected(10000))
        {
            Serial.println("Failed to connect. Make sure remote device is available and in range, then restart app.");
        }
    }

    while(!obd.begin(SerialBT, true, 2000))
    {
        Serial.println("Couldn't connect to OBD scanner");
    }

    Serial.println("Connected to ELM327");
}

void read_rpm()
{
    static unsigned long current_time{0};
    static unsigned long previous_rpm_time{0};
    static const int rpm_interval{112};

    current_time = millis();

    if (current_time - previous_rpm_time >= rpm_interval)
    {
        rpm = static_cast<short>(obd.rpm());
        Serial.print("RPM: "); Serial.println(rpm);
        previous_rpm_time = current_time;

        if ((obd.nb_rx_state != ELM_GETTING_MSG && obd.nb_rx_state != ELM_SUCCESS)
        {
            obd.printError();
        }
    }
}
