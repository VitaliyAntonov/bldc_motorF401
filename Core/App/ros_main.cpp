#include "ros_main.h"
#include "../Inc/main.h"

#include "../RosLibs/ros.h"
#include "../RosLibs/std_msgs/String.h"
#include "../RosLibs/sensor_msgs/Imu.h"


#include "../Process/Interval.h"

#include "../logger/Logger.hpp"

#include "../Inc/preference.h"
#include "../../Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_gpio.h"


void receiverCallback(const std_msgs::String & msg);
void recMotorBldc0(const PWR_MOTOR_MSG & msg);


/** массив данных об интервалах */
extern Interval arrIP;


extern Logger mLog;


extern TIM_HandleTypeDef htim1;

/** узел ROS */
ros::NodeHandle nh;


/** Публикатор */
std_msgs::String str_msg;
ros::Publisher chatter("kek", &str_msg);
char hello[] = "Hello world!";
char logMsgBuffer[50];

// Подписчики
/** Из обобщённого шаблона подписчика формируем объект подписчика для типа String */
ros::Subscriber <std_msgs::String> rec("lol", receiverCallback, 1);

/** motor1/pwr подписчик - мощность мотора 1 */
ros::Subscriber <PWR_MOTOR_MSG> motorBLDC0_pwr("bldc/pwr", recMotorBldc0, 1);

/** Обратный вызов при приёме сообщения */
void receiverCallback(const std_msgs::String & msg){
/** Здесь код выполняемый при получении сообщения  */
  chatter.publish(&msg);
}


/** Приведение типа сообщения PWR_MOTOR_MSG к типу int */
int msgRosToInt(PWR_MOTOR_MSG msg){
  /** умножаем пришедший float на 1000 и преобразуем в int */
  int pwm = static_cast <int> (msg.data * MAX_PWM_PULSE_COUNT);
  if(pwm > MAX_PWM_PULSE_COUNT) pwm = MAX_PWM_PULSE_COUNT;
  if(pwm < -MAX_PWM_PULSE_COUNT) pwm = -MAX_PWM_PULSE_COUNT;
  return pwm;
}

void setPwr(int pwr) {

  /** Остановка двигателя */
  if(pwr == 0){
    htim1.Instance->CCR1 = 0;
  }
  /** Двигатель вперёд */
  else if(pwr > 0) {
    HAL_GPIO_WritePin(BLDC_REVERS_GPIO_Port, BLDC_REVERS_Pin, GPIO_PIN_SET);
  }
  /** Двигатель назад */
  else{
    HAL_GPIO_WritePin(BLDC_REVERS_GPIO_Port, BLDC_REVERS_Pin, GPIO_PIN_RESET);
  }

  uint32_t mRegLoadPwm = static_cast <uint16_t> (abs(pwr)); /** Значение для загрузки в регистр ШИМ */

  /** Размещение задания мощности в диапазон MINIMUM_LIMIT_PWM ... MAXIMUM_LIMIT_PWM */
  uint32_t loadPwr = (mRegLoadPwm * (MAXIMUM_LIMIT_PWM - MINIMUM_LIMIT_PWM) /1000) + MINIMUM_LIMIT_PWM;


  htim1.Instance->CCR1 = static_cast <uint16_t> (loadPwr); /** Загружаем регистр ШИМ */
//  mPwrState = pwr;
}

/** Обратный вызов motor0_pwr */
void recMotorBldc0(const PWR_MOTOR_MSG & msg){

//  /** преобразуем и загружаем пришедшее значение мощности, как новое состояние двигателя */
//  dMotor.arrPwrm[NUM_MOTOR_0]->mPwrNewState = dMotor.msgRosToInt(msg);
//  /** Стартуем процесс изменения мощности */
//  dMotor.arrPwrm[NUM_MOTOR_0]->powerProcess->ss = MPDC_START;

  setPwr(msgRosToInt(msg));

  /** Отправка в ROS topic kek полученного сообщения */
  sprintf(logMsgBuffer, "%f", msg.data);
  str_msg.data = logMsgBuffer;
  chatter.publish(&str_msg);

  /** Здесь код выполняемый при получении сообщения  */
  HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
}


void ledToggle(){
  HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);

  str_msg.data = hello;
  chatter.publish(&str_msg);

  log_string(USB == Hello World - test message\r\n)

}

void txLoggerInterval(){
  mLog.transmitLogBuffer(); // отправка буфера лога в классе интервал
}

void linkSpinOnce(){
  nh.spinOnce();
}

/** Инициализация */
void setup(void){

/** Объекты класса интервал для функций основного цикла
 * Функция обработчика будет вызываться каждый раз через заданный интервал в миллисекундах
 * из loop() через arrIP.mainIntervalActivate()
 * посредством указателя на функцию */
  arrIP.createNewInterval(LOG_UART_TX_DMA_TIMEOUT, txLoggerInterval); // отправка лога
//  arrIP.createNewInterval(2000, ledToggle, 1);  // управление светодиодом
  arrIP.createNewInterval(998, linkSpinOnce, 2);  // основной отклик на ROS

  nh.initNode();               // инициализация узла ROS

  nh.advertise(chatter);    // инициализация издателя
  nh.subscribe(rec);        // инициализация подписчика

  nh.subscribe(motorBLDC0_pwr);  // тестирование BLDC двигателя - подписчик

  /** Запуск таймера 1 канал 1 как PWM mode для тестирования BLDC двигателя */
  HAL_TIM_Base_Start(&htim1);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
}


void loop(void){

  /** класс Interval проверка активации и вызов обработчиков */
  arrIP.mainIntervalActivate();

}
