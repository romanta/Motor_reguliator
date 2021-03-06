/*
  "3" релиз, делаем попытку добавить регулятор мощности на переменном резисторе
  Управление драйвером мотора,через ШИМ от датчика скорости колеса.
  В дальнейшем, такой мусор как подсчет скорости, можно выкинуть.
  В програме также присутствует индикация срабатывания датчиков скорости и каденса.
  Так же делаем попытку реализовать функцию подпрыгивания двигателя при старте.
  - Предел регулирования ШИМ "motor_spd" от 1000 до 2000
  - Значения тока "m_curent" от 511 до 725 или ток от 0 до 15000 мА.
  - таймер "delta_WH" 2000 нет движения, 1000 = 8.3 км/час, 111 82 км/час
  - таймер "delta_KD" 2000 нет движения, стартовое значение меньше 900 мс.
  минимальный ток холостого хода 1.5А
  максимальная мощность двигла 200 Ватт при токе 13 А
  Power должен біть равен от 523 до 740
*/


// Начало инициализации для серо
#include <Servo.h>
Servo motor;

const int Motor_Pin = 9;    //Подключаем контроллер мотора к пину 9
int motor_spd = 1000;  //Начальная позиция, всегда 1.0 мс для регуляторов бесколлекторных двигателей
float f_motor_spd;
int max_motor_spd = 2300; //Максимальное значение ШИМ 1.5 мс
int stop_motor_spd = 1000;  //Минимальное значени ШИМ 0.8 мс
// int start_volume = 400; // добавочная стартовая скорость мотора, тоесть при старте добавим это значение. чтобы двигатель прыгал.
int start_motor;
int start = 1;  //Флаг задержки запуска, если 1 то выполняем условие.
//Конец инициализации для серо

// Установки для регулятора мощности
int Pow_pin = A0;  // analog pin used to connect the potentiometer
int Power;    // variable to read the value from the analog pin
// конец блока

// Установки для тока
int Curent_pin = A1;
int m_curent; // сделал чтоб переменная тока не принимала минусовое значение
// конец блока для тока

const int Led_KAD = 6; //задаем ногу для светодиода кАДЕНСА. на ногу 6
const int Led_reed = 7; //задаем ногу для светодиода колеса. на ногу 7


// Настройки для колеса
const int reed = 3; //задаем ножку процессора для подключения счетчика на герконе 3 input
float radius = 340;//Радиус шины (миллиметрах) - изменить это для своего велосипеда
int reedVal_1;
long timer_wh = 0; // Время между одним полным оборотом (в мс)
float delta_WH;//буфер расчетов значений для ШИМ
float Kmh = 0.00;// скорость в КМ/час
float circumference;
int maxReedCounter = 100;// мин Время (в мс) одного вращения (для подавления  дребезга контактов)
int reedCounter;

// Настройка для каденс
const int reed2 = 4; //задаем ножку процессора для подключения счетчика на герконе
float radius2 = 340;//Радиус звезды (миллиметрах) - изменить это для своего велосипеда
int reedVal_2;
long timer_kd = 0; // Время между одним полным оборотом (в мс)
int delta_KD; //буфер для расчетов каденса
float Kmh2 = 0.00;// скорость в КМ/час
float circumference2;
int maxReedCounter2 = 20;// мин Время (в мс) одного вращения (для подавления  дребезга контактов)
int reedCounter2;

// Секция установок значений
void setup() {
  // For IDIOTE
  // initialize the LED pin as an output:
  pinMode(Pow_pin, INPUT);
  pinMode(Curent_pin, INPUT);
  pinMode(Led_KAD , OUTPUT);
  pinMode(Led_reed , OUTPUT);

  //Для мотора
  motor.attach(Motor_Pin, motor_spd, max_motor_spd);   //Инициальзация мотора (порт, начальная позиция, максимальная позиция)       !!!
  //Для 1 счетчика
  pinMode(reed, INPUT); //присваеиваем переменной привязанной ко к ноге значение ВХОДНОЙ
  reedCounter = maxReedCounter;
  circumference = (2 * 3.415926535 * radius) / 1000000; // расчет скоростей
  // Для 2 счетчика
  pinMode(reed2, INPUT); //присваеиваем переменной привязанной ко к ноге значение ВХОДНОЙ
  reedCounter2 = maxReedCounter2;
  circumference2 = (2 * 3.415926535 * radius2) / 1000000; // расчет скоростей

  // НАЧАЛО УСТАНОВКИ ТАЙМЕРОВ
  // ТАЙМЕР SETUP- прерывание таймера позволяет точно синхронизированные измерения геркона
  // для получения дополнительной информации о конфигурации Arduino таймеров см http://arduino.cc/playground/Code/Timer1
  cli();//stop interrupts

  //set timer3 interrupt at 1kHz, Из того что понял, мы настраеваем таймер на 2000 = 2000 мс,
  //если происходит совпадение срабатывания холла в этом интервале то все ОК, если выходит за интервал 2000 мс,
  //то происходит переполнение буфера, и таймер сбрасывается и считает сначала.
  TCCR3A = 0;// Обнуляем регистры таймера TCCR3A
  TCCR3B = 0;// Обнуляем регистры таймера TCCR3B
  TCNT3  = 0;// Обнуляем регистры
  OCR3A = 1999;// = (1/1000) / ((1/(16*10^6))*8) - 1 // Установить счетчик таймера с шагом 1 кГц
  TCCR3B |= (1 << WGM32); // установитьь для таймера TCCR3A, CTC режим (сброс при совпадении OCR3A)
  TCCR3B |= (1 << CS31);    // установить делитель таймера TCCR3B режимом  CS31 в 1 (частота контроллера / на 8)
  TIMSK3 |= (1 << OCIE3A);  // Бит 4 - OCIE3A: прерывание по совпадению A ТС3

  //set timer4 interrupt at 1 kHz
  TCCR4A = 0;// Обнуляем регистры таймера TCCR0A
  TCCR4B = 0;// Обнуляем регистры таймера TCCR0B
  TCNT4  = 0;// Обнуляем регистры
  OCR4A = 1999;// = (1/1000) / ((1/(16*10^6))*8) - 1 // Установить счетчик таймера с шагом 1 кГц
  TCCR4B |= (1 << WGM41); // установитьь для таймера TCCR0A, CTC режим (сброс при совпадении OCR0A)
  TCCR4B |= (1 << CS42);    // установить делитель таймера TCCR4B режимом  CS42 в 1 (частота контроллера / на 8)
  TIMSK4 |= (1 << OCIE4A);  // Бит 4 - OCIE4A: прерывание по совпадению A ТС4

  sei();//разрешить все прерывания
  //КОНЕЦ НАСТРОЙКИ ТАЙМЕРОВ


  Serial.begin(9600);//Настройка для вывода значений в сериал порт.
}


// Начало подсчета для каденса
/*
    Производим вычисление величины временных интервалов для частоты педалирования
*/
ISR(TIMER4_COMPA_vect) {// Прерывание при частоте 1 кГц для измерения геркона
  reedVal_2 = digitalRead(reed2);// получить значение датчика
  if (reedVal_2 == 1) { // если геркон замкнут
    if (reedCounter2 == 0) { // время между импульсами мин прошло
      Kmh2 = float(circumference2) / float(timer_kd) * 3600000; // вычисляем скорость в М/с
      delta_KD = timer_kd;// Пприсвоить переменной дельте значение таймера колеса
      timer_kd = 0;//сбросить таймер
      reedCounter2 = maxReedCounter2;// сбросить reedCounter в заданное вначале значение
    }
    else {
      if (reedCounter2 > 0) { // не дает стать отрицательным значением reedCounter
        reedCounter2 -= 1;//уменьшение reedCounter
      }
    }
  }
  else { //сли геркон открыт
    if (reedCounter2 > 0) { // не дает стать отрицательным значением reedCounter
      reedCounter2 -= 1;//уменьшение reedCounter
    }
  }
  if (timer_kd > 2000) { // Если время накапало больше 2000 мс
    Kmh2 = 0;//если нет новых импульсов от каденса - колеса до сих пор, установите скорость до 0
    delta_KD = timer_kd; // то значение дельты тоже будет равно 2000, дабы избежать зависших значений дельты, при отстановке педалей
  }
  else {
    timer_kd += 1;//увеличиваем timer
    //   delta_KD += 1;
  }

  // Тут кусок кода для мигания светодиодом, для проверки работы датчиков
  if (reedVal_2 == HIGH) { //
    // turn LED on://
    digitalWrite(Led_KAD, HIGH);//
  }//
  else {//
    // turn LED off://
    digitalWrite(Led_KAD, LOW);//
  }//
  // конец кода мигалки

}
// Конец подсчета для каденса


//Начало подсчета для колеса
ISR(TIMER3_COMPA_vect) {// Прерывание при частоте 1 кГц для измерения геркона
  reedVal_1 = digitalRead(reed);// получить значение датчика
  if (reedVal_1 == 1) { // если геркон замкнут
    if (reedCounter == 0) { // время между импульсами мин прошло
      Kmh = float(circumference) / float(timer_wh) * 3600000; // вычисляем скорость в М/с
      delta_WH = timer_wh;// Пприсвоить переменной дельте значение таймера колеса
      timer_wh = 0;//сбросить таймер
      reedCounter = maxReedCounter;// сбросить reedCounter в заданное вначале значение
    }
    else {
      if (reedCounter > 0) { // не дает стать отрицательным значением reedCounter
        reedCounter -= 1;//уменьшаем reedCounter
      }
    }
  }
  else { //сли геркон открыт
    if (reedCounter > 0) { // не дает стать отрицательным значением reedCounter
      reedCounter -= 1;//уменьшаем reedCounter
    }
  }

  if (timer_wh > 2000) {
    Kmh = 0;//если нет новых импульсов от Reed выключателя - колеса до сих пор, установите скорость до 0
    motor_spd = stop_motor_spd; // Если таймер больше 2000 то пишем в порт мотора нулевую скорость.
    delta_WH = timer_wh;// и присваиваем значение дельте 2000
  }
  else {
    timer_wh += 1;//увеличиваем timer
    //  delta_WH += 1;
  }

  // Тут кусок кода для мигания светодиодом, для проверки работы датчиков
  if (reedVal_1 == HIGH) {
    // turn LED on:
    digitalWrite(Led_reed, HIGH);
  }
  else {
    // turn LED off:
    digitalWrite(Led_reed, LOW);
  }
  // конец кода мигалки



}

// Конец посчета для колеса


void loop() {


  //Стартовая цикличность
  if (start == 1) {
    motor.write(stop_motor_spd);
    delay(1000); // задержка для инициилизации контроллера двигателя
    start = 0; // обнуляем значение старта, для продолжения дальнейшей работы.
  }


  Power = analogRead(Pow_pin);            // считывает значение потенциометра (значение от 0 до 1023)
  Power = map(Power, 0, 1023, 500, 723);     // масштабировать его для использования с сервоприводом 
  //потом надо включить данное значение Power в обработку для ШИМа

  // Начало преобразования задержки в значение для выхода ШИМ
  m_curent = analogRead(Curent_pin); // читаем значение с  пина датчика тока
  m_curent = constrain(m_curent, 520, 750); // ограничиваем переменную в преднлах данных значений
  // m_curent  = map(m_curent, 509, 1023, 0, 29000); // преобразуем значение датчика тока в удобоваримый формат тока / Датчик на 30 А но юзаем только 15 А

  //  if  (start_motor = 1){
  //       motor_spd = (motor_spd + start_volume);
  //       delay(1000); // задержка для инициилизации контроллера двигателя
  //       start_motor = 0;
  //

  // Начало преобразования задержки в значение для выхода ШИМ
  if (delta_KD > 2003 || delta_WH >= 1000 || delta_WH <= 110) { //если значение таймера дельты2 больше "" или дельты1 больше ""
    motor_spd = stop_motor_spd;//то пишем в порт мотора нулевую скорость.
    motor.write(motor_spd);
    start_motor = 0;
  }

  else {
    if (delta_KD < 2002) {
      //НАДО ВВЕСТИ ПЕРЕМЕННУЮ ДЛЯ ДЕЛЬТЫ ТОКА, КОТОРАЯ ЗАВИСИТ ОТ ДЕЛЬТЫ СКОРОСТИ
      f_motor_spd = map(m_curent, 750, 523, 1000, 1900);  //Преобразование задержки между (мс) импульсами геркона таймера1 - в миллисекунды для ШИМа

      // тут нужно условие для  Power. если m_curent меньше или больше заданного значения  Power, то соответственно либо прибавляем f_motor_spd по еденице, либо отнимаем.
      if (m_curent > Power) {
        f_motor_spd -= 1;//уменьшаемем обоороты
      }
      if (m_curent < Power) {
        f_motor_spd += 1;//увеличиваем обоороты
      }
      else  {
        f_motor_spd = f_motor_spd;//
      }

      motor_spd = f_motor_spd;
      motor.write(motor_spd);
      start_motor = 1;
      delay(10); // задержка для плавности работы двигателя
    }
  }



  // Serial.print(Kmh);  Serial.println(" Kmh");// после отладки отключить
  Serial.print(start_motor);  Serial.println(" start_motor");// после отладки отключить
  Serial.print( m_curent);  Serial.println("  m_curent");// после отладки отключить
  Serial.print(delta_WH);  Serial.println(" delta_WH");// после отладки отключить
  //  Serial.print(delta_KD);  Serial.println(" delta_KD");// после отладки отключить
  //  //  Serial.print(f_motor_spd);  Serial.println(" f_motor_spd");// после отладки отключить
  Serial.print(Power);  Serial.println(" Power");// после отладки отключить
  Serial.print(motor_spd);  Serial.println(" motor_spd");// после отладки отключить
  delay(1a00);
}

