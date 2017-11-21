#include <avr/wdt.h>
#include <avr/power.h>
// USB
#include <hidboot.h>
#include <usbhub.h>
#include <SPI.h>
const int ADELAY = 200;
USB Usb;
USBHub Hub(& Usb);
HIDBoot < USB_HID_PROTOCOL_MOUSE > HidMouse(& Usb);

uint8_t HQ[4] = { 0, 1, 1, 0 };
uint8_t H[4] =  { 0, 0, 1, 1 };

uint8_t QX = 3;
uint8_t QY = 3;
uint8_t XSTEPS;
uint8_t YSTEPS;
uint8_t XSIGN;
uint8_t YSIGN;
// Quadrature mouse output port
// Right mouse button - pin7
#define RB_PORT PORTD
#define RB_PIN PIND
#define RB_DDR DDRD
#define RB (1 << 7)

// Middle mouse button - pin8
#define MB_PORT PORTB
#define MB_PIN PINB
#define MB_DDR DDRB
#define MB (1 << 0)
// Left mouse button - pin6
#define LB_PORT PORTD
#define LB_PIN PIND
#define LB_DDR DDRD
#define LB (1 << 6)
// Y axis output - pin2
#define Y2_PORT PORTD
#define Y2_PIN PIND
#define Y2_DDR DDRD
#define Y2 (1 << 2)
// pin3
#define Y1_PORT PORTD
#define Y1_PIN PIND
#define Y1_DDR DDRD
#define Y1 (1 << 3)
// X axis output - pin4
#define X2_PORT PORTD
#define X2_PIN PIND
#define X2_DDR DDRD
#define X2 (1 << 4)
// pin5
#define X1_PORT PORTD
#define X1_PIN PIND
#define X1_DDR DDRD
#define X1 (1 << 5)
void initialiseHardware(void)
{
  // Disable the watchdog timer (if set in fuses)
  MCUSR &= ~(1 << WDRF);
  wdt_disable();
  // Disable the clock divider (if set in fuses)
  clock_prescale_set(clock_div_1);
  // Set the quadrature output pins to output
  X1_DDR |= X1; // Output
  X2_DDR |= X2; // Output
  Y1_DDR |= Y1; // Output
  Y2_DDR |= Y2; // Output
  LB_DDR |= LB; // Output
  MB_DDR |= MB; // Output
  RB_DDR |= RB; // Output
  // Set quadrature output pins to zero
  X1_PORT &= ~X1; // Pin = 0
  X2_PORT &= ~X2; // Pin = 0
  Y1_PORT &= ~Y1; // Pin = 0
  Y2_PORT &= ~Y2; // Pin = 0
  // Set mouse button output pins to on
  // Note: Mouse buttons are inverted, so this sets them to 'off'
  // from the host's perspective
  LB_PORT |= LB; // Pin = 1 (on)
  MB_PORT |= MB; // Pin = 1 (on)
  RB_PORT |= RB; // Pin = 1 (on)
}

class MouseRptParser: public MouseReportParser
{
  protected:
    void OnMouseMove(MOUSEINFO * mi);
    void OnLeftButtonUp(MOUSEINFO * mi);
    void OnLeftButtonDown(MOUSEINFO * mi);
    void OnRightButtonUp(MOUSEINFO * mi);
    void OnRightButtonDown(MOUSEINFO * mi);
    void OnMiddleButtonUp(MOUSEINFO * mi);
    void OnMiddleButtonDown(MOUSEINFO * mi);
};

void MouseRptParser::OnMouseMove(MOUSEINFO * mi)
{
  XSTEPS = abs(mi -> dX);
  YSTEPS = abs(mi -> dY);
  XSIGN = (mi -> dX < 0 ? 1 : 0) & B1;
  YSIGN = (mi -> dY > 0 ? 1 : 0) & B1;
}

void MouseRptParser::OnLeftButtonUp(MOUSEINFO * mi)
{
  LB_PORT |= LB;
}

void MouseRptParser::OnLeftButtonDown(MOUSEINFO * mi)
{
  LB_PORT &= ~LB;
}

void MouseRptParser::OnRightButtonUp(MOUSEINFO * mi)
{
  // RB_PORT |= RB;
  RB_PORT &= ~RB;
}

void MouseRptParser::OnRightButtonDown(MOUSEINFO * mi)
{
  // RB_PORT &= ~RB;
   RB_PORT |= RB;
} 

void MouseRptParser::OnMiddleButtonUp(MOUSEINFO * mi)
{
  MB_PORT |= MB;
}

void MouseRptParser::OnMiddleButtonDown(MOUSEINFO * mi)
{
  MB_PORT &= ~MB;
}

MouseRptParser Prs;
void AMIGAHorizontalMove()
{
  PORTD = (PORTD & B11110011) | ((H[QX] << PORTD2) | (HQ[QX] << PORTD3));
  delayMicroseconds(ADELAY);
}

void AMIGAVerticalMove()
{
  PORTD = (PORTD & B11001111) | ((H[QY] << PORTD4) | (HQ[QY] << PORTD5));
  delayMicroseconds(ADELAY);
}

void AMIGALeft()
{
  AMIGAHorizontalMove();
  QX = (QX >= 3) ? 0 : ++QX;
}

void AMIGARight()
{
  AMIGAHorizontalMove();
  QX = (QX <= 0) ? 3 : --QX;
}

void AMIGADown()
{
  AMIGAVerticalMove();
  QY = QY <= 0 ? 3 : --QY;
}

void AMIGAUp()
{
  AMIGAVerticalMove();
  QY = QY >= 3 ? 0 : ++QY;
}

void setup()
{
  noInterrupts(); // disable all interrupts
  pinMode(LED_BUILTIN, OUTPUT);
  MCUSR &= ~(1 << WDRF);
  wdt_disable();
  initialiseHardware();
  interrupts(); // enable all interrupts
  if (Usb.Init() == -1)
    while (1);
  delay(200);
  HidMouse.SetReportParser(0, & Prs);
}

void loop()
{
  Usb.Task();
  while ((XSTEPS | YSTEPS) != 0)
  {
    // Movement
    if (XSTEPS != 0)
    {
      if (XSIGN)
        // X SIGN
        AMIGALeft();
      else
        AMIGARight();
      XSTEPS--;
    }
    if (YSTEPS != 0)
    {
      if (YSIGN)
        // Y SIGN
        AMIGADown();
      else
        AMIGAUp();
      YSTEPS--;
    }
  }
  // while
}
