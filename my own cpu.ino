#include <LiquidCrystal.h>

template <typename T_ty> struct TypeInfo { static const char * name; };
template <typename T_ty> const char * TypeInfo<T_ty>::name = "unknown";

// Handy macro to make querying stuff easier.
#define TYPE_NAME(var) TypeInfo< typeof(var) >::name

// Handy macro to make defining stuff easier.
#define MAKE_TYPE_INFO(type)  template <> const char * TypeInfo<type>::name = #type;

const int rs = 12, en = 11, d4 = A0, d5 = A1, d6 = A2, d7 = 13;
const int rst = 2;
char reservedSerialChars[5] = {1, 2, 3, 4, 27};
const int KeyDefsArray[2][4] = {
  {3, 4, 5, 6},
  {7, 8, 9, 10}
};
const char keyRedr[] = {1, 2, 3, 0xa, 4, 5, 6, 0xb, 7, 8, 9, 0xc, 0xe, 0, 0xf, 0xd};
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;

int positive_modulo(int i, int n) {
  return (i % n + n) % n;
}

typedef struct TListItem *PListItem;
struct TListItem {
public:
  void *item;
  PListItem after;
  const char *type;
};

class Queue {
public:
  int length() {
    return this->len;
  };
  void append(void* item, const char* type) {
    PListItem obj = new TListItem();
    obj->item = item;
    obj->type = type;
    this->len++;
    this->TAIL->after = obj;
    this->TAIL = obj;
  };
protected:
  int len;
  PListItem HEAD;
  PListItem TAIL;
};

struct SerialInfo {
  char type[3] = {0, 0, 0};
  char* message;
  unsigned int length = 0;
};

struct Interrupt {
  bool enabled;
  uint address;
  uchar condition;
};

class KeyDefsType {
  public:
  KeyDefsType()
  {
    this->c1 = KeyDefsArray[0][0];
    this->c2 = KeyDefsArray[0][1];
    this->c3 = KeyDefsArray[0][2];
    this->c4 = KeyDefsArray[0][3];
    this->r1 = KeyDefsArray[1][0];
    this->r2 = KeyDefsArray[1][1];
    this->r3 = KeyDefsArray[1][2];
    this->r4 = KeyDefsArray[1][3];
  };
  int c1;
  int c2;
  int c3;
  int c4;
  int r1;
  int r2;
  int r3;
  int r4;
};

KeyDefsType KeyDefs;

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void clearScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
}

void SerialSend(char type[3], char message[], unsigned int length) {
  Serial.print("\x01");
  Serial.print(type);
  Serial.print("\x03");
  Serial.print("\x02");
  for (int i = 0; i < length; i++) {
      if (objInArray(message[i], reservedSerialChars, 5)) {
        Serial.write(27);
      }
      Serial.write(message[i]);
  }
  Serial.print("\x04");
}

void SerialSendNoMessage(char type[3]) {
  Serial.print("\x01");
  Serial.print(type);
  Serial.print("\x03\x04");
}

void halt() {
  while (1) {};
}

void softReset() {
  Serial.end();
  setup();
  while (1) {loop();}
}

void hardReset() {
  analogWrite(rst, 0);
  analogWrite(rst, 255);  
}

SerialInfo SerialRead() {
  while (Serial.available() < 6) {};
  char type[3];
  for (int i = 0; i < 5; i++) {
    if (i != 0 || i != 4) {
      type[i - 1] = Serial.read();
    } else {
      Serial.read();
    }
  }
  int sr = Serial.read();
  SerialInfo si = SerialInfo();
  for (int i = 0; i < 3; i++) {
    si.type[i] = type[i];
  }
  if (sr == 4) {
    return si;
  } else if (sr == 2) {
    bool cond = 1;
    bool newcond = 1;
    char message[256];
    si.message = (char*)(&message);
    while (cond) {
      sr = Serial.read();
      switch (sr) {
        case 4:
          Serial.println("Exited.");
          cond = 0;
          break;
        case 27:
          int newsr;
          while (newcond) {
            newsr = Serial.read();
            switch (newsr) {
              case -1:
                break;
              default:
                message[si.length] = sr;
                si.length++;
                newcond = 0;
                break;
            }
          }
          newcond = 1;          
          break;
        case -1:
          break;
        default:
          message[si.length] = sr;
          si.length++;
          break;
      }
    }
    return si;
  } else {
    return si;
  }
}

template <typename T>
bool objInArray(T obj, T arr[], unsigned int length) {
  for (int i = 0; i < length; i++) {
    if (arr[i] == obj) {
      return true;
    }
  }
  return false;
}

void setup() {
  analogWrite(rst, 255);
  lcd.begin(20, 4);
  pinMode(KeyDefs.c1, OUTPUT);
  pinMode(KeyDefs.c2, OUTPUT);
  pinMode(KeyDefs.c3, OUTPUT);
  pinMode(KeyDefs.c4, OUTPUT);
  pinMode(KeyDefs.r1, INPUT);
  pinMode(KeyDefs.r2, INPUT);
  pinMode(KeyDefs.r3, INPUT);
  pinMode(KeyDefs.r4, INPUT);
  digitalWrite(KeyDefs.c1, 0);
  digitalWrite(KeyDefs.c2, 0);
  digitalWrite(KeyDefs.c3, 0);
  digitalWrite(KeyDefs.c4, 0);
  Serial.begin(9600);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("If you see this then");
  lcd.setCursor(0, 1);
  lcd.print("USB is not connected");
  SerialSendNoMessage("RST");
  bool e = false;
  while (1) {
    SerialInfo si = SerialRead();
    if (si.length == 3 && si.type == "RST") {
      if (si.message == "ACK") {
        break;
      }
    } else {
      e = true;
    }
  }
  if (e) {
    delay(100);
    softReset();
  }
}
bool *KeyboardMatrix() {
  bool keys[16];
  for (int c = 0; c < 4; c++) {
    analogWrite(KeyDefsArray[0][c], 255);
    for (int r = 0; r < 4; r++) {
      keys[keyRedr[c + r * 4]] = (bool)digitalRead(KeyDefsArray[1][r]);
    }
    analogWrite(KeyDefsArray[0][c], 0);
  }
  return keys;
}
typedef class Memory* MemoryP;
typedef class ExpansionInterface* ExpansionInterfaceP;
typedef class StackMemory* StackMemoryP;
typedef class ALU* ALUP;
typedef class Emulator* EmulatorP;
class Emulator
{
public:
  unsigned int programCounter;
  unsigned int address;
  MemoryP mem;
  ExpansionInterfaceP exp;
  StackMemoryP stackmem;
  ALUP alu;
  bool inInterrupt;
  Interrupt intrpt[2];
  uchar registers[64];
  Emulator() {
    this->programCounter = 0;
    this->address = 0;
    for (int i = 0; i < 64; i++) {
      this->registers[i] = 0;
    }
  };
  void init();
  void run();
  void reset() {
    hardReset();
  };
  bool cond(uchar cond);
  void enterInterrupt(bool interrupt);
};
typedef Emulator* EmulatorP;

class Memory
{
public:
  Memory(EmulatorP emu) {
    this->emu = emu;
  }
  uchar read(unsigned int adr) {
    String str = String(adr);
    unsigned int len = str.length();
    void* voidobj = malloc(len);
    if (voidobj == NULL) {
      SerialSend("FER", "A fatal error has occurred allocating memory for a memory read operation. System halted.", 88);
      halt();
    }
    char* obj = (char*)voidobj;
    str.toCharArray(obj, len);
    SerialSend("MER", obj, len);
    free(obj);
    free(voidobj);
    SerialInfo sr = SerialRead();
    if (sr.type != "MER") {
      SerialSend("FER", "Unexpected serial type sent. System halted.", 43);
      halt();
    }
    return (uchar)String(sr.message).toInt();
  }
  uchar* read4(unsigned int programCounter) {
    String str = String(programCounter);
    unsigned int len = str.length();
    void* voidobj = malloc(len);
    if (voidobj == NULL) { SerialSend("FER", "A fatal error has occurred allocating memory for a memory read operation. System halted.", 88); halt();}
    char* obj = (char*)voidobj;
    str.toCharArray(obj, len);
    SerialSend("MEI", obj, len);
    free(obj);
    free(voidobj);
    SerialInfo sr = SerialRead();
    if (sr.type != "MEI") {SerialSend("FER", "Unexpected serial type sent. System halted.", 43);halt();}
    uchar arr[4];
    arr[0] = String(sr.message).toInt();
    sr = SerialRead();
    if (sr.type != "MEI") {SerialSend("FER", "Unexpected serial type sent. System halted.", 43);halt();}
    arr[1] = String(sr.message).toInt();
    sr = SerialRead();
    if (sr.type != "MEI") {SerialSend("FER", "Unexpected serial type sent. System halted.", 43);halt();}
    arr[2] = String(sr.message).toInt();
    sr = SerialRead();
    if (sr.type != "MEI") {SerialSend("FER", "Unexpected serial type sent. System halted.", 43);halt();}
    arr[3] = String(sr.message).toInt();
    return arr;
  }
  void write(unsigned int adr, unsigned char data) {
    String str = String(adr);
    str.concat('|');
    str.concat(String(data));
    unsigned int len = str.length();
    void* voidobj = malloc(len);
    if (voidobj == NULL) {
      SerialSend("FER", "A fatal error has occurred allocating memory for a memory read operation. System halted.", 88);
      halt();
    }
    char* obj = (char*)voidobj;
    str.toCharArray(obj, len);
    SerialSend("MEI", obj, len);
    free(obj);
    free(voidobj);
  }
  ushort getSizeK() {
    SerialSendNoMessage("MSK");
    SerialInfo sr = SerialRead();
    if (sr.type != "MSK") {
      SerialSend("FER", "Unexpected serial type sent. System halted.", 43);
      halt();
    }
    return (ushort)String(sr.message).toInt();
  }
  bool ovfw;
  bool autoincrement;
private:
  EmulatorP emu;
};

class ExpansionInterface
{
public:
  ExpansionInterface(EmulatorP emu) {
    this->emu = emu;
  };
  void reset(uchar expansion) {
    String str = String(expansion);
    unsigned int len = str.length();
    void* voidobj = malloc(len);
    if (voidobj == NULL) {
      SerialSend("FER", "A fatal error has occurred allocating memory for a memory read operation. System halted.", 88);
      halt();
    }
    char* obj = (char*)voidobj;
    str.toCharArray(obj, len);
    SerialSend("EXQ",obj,len);
    free(obj);
    free(voidobj);
    
  };
  uchar get(uchar expansion) {
    String str = String(expansion);
    unsigned int len = str.length();
    void* voidobj = malloc(len);
    if (voidobj == NULL) {
      SerialSend("FER", "A fatal error has occurred allocating memory for a memory read operation. System halted.", 88);
      halt();
    }
    char* obj = (char*)voidobj;
    str.toCharArray(obj, len);
    SerialSend("EXG",obj,len);
    free(obj);
    free(voidobj);
    SerialInfo sr = SerialRead();
    if (sr.type != "EXG") {
      SerialSend("FER", "Unexpected serial type sent. System halted.", 43);
      halt();
    }
    return (uchar)String(sr.message).toInt();
  };
  uchar read(uchar expansion) {
    String str = String(expansion);
    unsigned int len = str.length();
    void* voidobj = malloc(len);
    if (voidobj == NULL) {
      SerialSend("FER", "A fatal error has occurred allocating memory for a memory read operation. System halted.", 88);
      halt();
    }
    char* obj = (char*)voidobj;
    str.toCharArray(obj, len);
    SerialSend("EXR",obj,len);
    free(obj);
    free(voidobj);
    SerialInfo sr = SerialRead();
    if (sr.type != "EXR") {
      SerialSend("FER", "Unexpected serial type sent. System halted.", 43);
      halt();
    }
    return (uchar)String(sr.message).toInt();
  };
  void send(uchar expansion, uchar data) {
    Serial.print("\x01");
    Serial.print("EXS");
    Serial.print("\x03");
    Serial.print("\x02");
    Serial.print(expansion);
    Serial.print("|");
    Serial.print(data);
    Serial.print("\x04");
  };
  uchar wait(uchar expansion) {
    String str = String(expansion);
    unsigned int len = str.length();
    void* voidobj = malloc(len);
    if (voidobj == NULL) {
      SerialSend("FER", "A fatal error has occurred allocating memory for a memory read operation. System halted.", 88);
      halt();
    }
    char* obj = (char*)voidobj;
    str.toCharArray(obj, len);
    SerialSend("EXW",obj,len);
    free(obj);
    free(voidobj);
    SerialInfo sr = SerialRead();
    if (sr.type != "EXW") {
      SerialSend("FER", "Unexpected serial type sent. System halted.", 43);
      halt();
    }
    return (uchar)String(sr.message).toInt();
  };
  void resetAll() {
    SerialSendNoMessage("EXF");
  };
  ushort getID(uchar expansion) {
    String str = String(expansion);
    unsigned int len = str.length();
    void* voidobj = malloc(len);
    if (voidobj == NULL) {
      SerialSend("FER", "A fatal error has occurred allocating memory for a memory read operation. System halted.", 88);
      halt();
    }
    char* obj = (char*)voidobj;
    str.toCharArray(obj, len);
    SerialSend("EXI",obj,len);
    free(obj);
    free(voidobj);
    SerialInfo sr = SerialRead();
    if (sr.type != "EXI") {
      SerialSend("FER", "Unexpected serial type sent. System halted.", 43);
      halt();
    }
    return (ushort)String(sr.message).toInt();
  };
  bool ping(uchar expansion) {
    String str = String(expansion);
    unsigned int len = str.length();
    void* voidobj = malloc(len);
    if (voidobj == NULL) {
      SerialSend("FER", "A fatal error has occurred allocating memory for a memory read operation. System halted.", 88);
      halt();
    }
    char* obj = (char*)voidobj;
    str.toCharArray(obj, len);
    SerialSend("EXP",obj,len);
    free(obj);
    free(voidobj);
    SerialInfo sr = SerialRead();
    if (sr.type != "EXP") {
      SerialSend("FER", "Unexpected serial type sent. System halted.", 43);
      halt();
    }
    return (bool)String(sr.message).toInt();
  };
  void setAddress(uchar expansion) {
    this->address = expansion;
  };
  bool exists() {
    String str = String(this->address);
    unsigned int len = str.length();
    void* voidobj = malloc(len);
    if (voidobj == NULL) {
      SerialSend("FER", "A fatal error has occurred allocating memory for a memory read operation. System halted.", 88);
      halt();
    }
    char* obj = (char*)voidobj;
    str.toCharArray(obj, len);
    SerialSend("EXE",obj,len);
    free(obj);
    free(voidobj);
    SerialInfo sr = SerialRead();
    if (sr.type != "EXE") {
      SerialSend("FER", "Unexpected serial type sent. System halted.", 43);
      halt();
    }
    return (bool)String(sr.message).toInt();
  };
  bool hasReply() {
    String str = String(this->address);
    unsigned int len = str.length();
    void* voidobj = malloc(len);
    if (voidobj == NULL) {
      SerialSend("FER", "A fatal error has occurred allocating memory for a memory read operation. System halted.", 88);
      halt();
    }
    char* obj = (char*)voidobj;
    str.toCharArray(obj, len);
    SerialSend("EXR",obj,len);
    free(obj);
    free(voidobj);
    SerialInfo sr = SerialRead();
    if (sr.type != "EXR") {
      SerialSend("FER", "Unexpected serial type sent. System halted.", 43);
      halt();
    }
    return (bool)String(sr.message).toInt();
  };
  uchar address;
private:
  EmulatorP emu;
};

class ALU
{
public:
  ALU(EmulatorP emu) {
    this->emu = emu;
  };
  uchar A;
  uchar B;
  bool ovfw;
  uchar IAR(uchar reg) {
    this->ovfw = reg == 255;
    return (reg + 1);
  };
  uchar DAR(uchar reg) {
    this->ovfw = reg == 0;
    return (reg - 1);
  }
  uchar ADD(uchar a, uchar b) {
    ushort out = (ushort)(a + b);
    this->ovfw = out > 255;
    return (uchar)out;
  }
  uchar SUB(uchar a, uchar b) {
    short out = (short)(a - b);
    this->ovfw = out < 0;
    return (uchar)positive_modulo(out, 256);
  }
  uchar MUL(uchar a, uchar b) {
    ushort out = (ushort)(a * b);
    this->ovfw = out > 255;
    return (uchar)out;
  }
  uchar MOD(uchar a, uchar b) {
    this->ovfw = 0;
    return (a % b);
  }
  uchar DIV(uchar a, uchar b) {
    this->ovfw = 0;
    return (a / b);
  }
  uchar SHL(uchar a) {
    ushort out = (a << 1);
    this->ovfw = out > 255;
    return (uchar)out;
  }
  uchar SHR(uchar a) {
    this->ovfw = 0;
    return a >> 1;
  }
  uchar AND(uchar a, uchar b) {
    this->ovfw = 0;
    return a & b;
  }
  uchar OR(uchar a, uchar b) {
    this->ovfw = 0;
    return a | b;
  }
  uchar XOR(uchar a, uchar b) {
    this->ovfw = 0;
    return a ^ b;
  }
  uchar NAND(uchar a, uchar b) {
    this->ovfw = 0;
    return ~(a & b);
  }
  uchar NOR(uchar a, uchar b) {
    this->ovfw = 0;
    return ~(a | b);
  }
  uchar XNOR(uchar a, uchar b) {
    this->ovfw = 0;
    return ~(a ^ b);
  }
  uchar NOT(uchar a) {
    this->ovfw = 0;
    return ~a;
  }
  void SAB(uchar a, uchar b) {
    this->A = a;
    this->B = b;
  }

private:
  EmulatorP emu;
};

class StackMemory
{
public:
  StackMemory(EmulatorP emu) {
    this->emu = emu;
    this->pointer = 0;
  };
  uchar pointer;
  void push(uchar data) {
    this->mem[this->pointer] = data;
    this->pointer++;
  }
  uchar pull() {
    this->pointer--;
    return this->mem[this->pointer];
  }
  void pushaddr(uint address) {
    this->push((uchar)(address >> 16));
    this->push((uchar)(address >> 8));
    this->push((uchar)(address));
  }
  uint pulladdr() {
    uchar arr[4];
    arr[0] = this->pull();
    arr[1] = this->pull();
    arr[2] = this->pull();
    return ((uint*)arr)[0];
  }
private:
  EmulatorP emu;
  uchar mem[256];
};

class KeyboardHandler
{
public:
  KeyboardHandler(EmulatorP emu) {
    this->emu = emu;
  };
  void enable() {
    this->enabled = true;
  };
  void disable() {
    this->enabled = false;
  };

private:
  EmulatorP emu;
  bool enabled = false;
};

inline void Emulator::init() {
  this->mem = new Memory(this);
  this->exp = new ExpansionInterface(this);
  this->alu = new ALU(this);
}
typedef bool (*ConditionEvaluator) (EmulatorP emu);
ConditionEvaluator conditions[15];

inline void Emulator::run() {
  while (1) {
    if (!(this->inInterrupt)) {
      if (this->intrpt[0].enabled) {
        if (this->cond(this->intrpt[0].condition)) {
          this->enterInterrupt(0);
        }
      }
    }
    if (!(this->inInterrupt)) {
      if (this->intrpt[1].enabled) {
        if (this->cond(this->intrpt[1].condition)) {
          this->enterInterrupt(1);
        }
      }
    }
    uchar* arr = this->mem->read4(this->programCounter);
    uchar inst = arr[0]; uchar arg1 = arr[1]; uchar arg2 = arr[2]; uchar arg3 = arr[3];
    ushort temp;
    switch (inst) {
      case 0:
        halt();
      case 1:
        this->programCounter++;
        break;
      case 2:
        hardReset();
        break;
      case 3:
        this->intrpt[0].enabled = true;
        this->programCounter++;
        break;
      case 4:
        this->intrpt[0].enabled = false;
        this->programCounter++;
        break;
      case 5:
        this->programCounter++;
        this->enterInterrupt(0);
        break;
      case 6:
        this->intrpt[0].condition = this->registers[arg1 % 64];
        this->programCounter += 2;
        break;
      case 7:
        this->intrpt[0].address = ((this->intrpt[0].address >> 8) << 8) + this->registers[arg1 % 64];
        this->programCounter += 2;
        break;
      case 8:
        this->intrpt[0].address = this->registers[arg1 % 64] * 256 + (this->intrpt[0].address % 256) + ((this->intrpt[0].address >> 16) << 16);
        this->programCounter += 2;
        break;
      case 9:
        this->intrpt[0].address = this->intrpt[0].address - ((this->intrpt[0].address >> 16) << 16) + this->registers[arg1 % 64] * 65536;
        this->programCounter += 2;
        break;
      case 10:
        this->intrpt[0].address = this->registers[arg1 % 64] + this->registers[arg2 % 64] * 256 + this->registers[arg3 % 64] * 65536;
        this->programCounter += 4;
        break;
      case 11:
        this->intrpt[1].enabled = true;
        this->programCounter++;
        break;
      case 12:
        this->intrpt[1].enabled = false;
        this->programCounter++;
        break;
      case 13:
        this->programCounter++;
        this->enterInterrupt(1);
        break;
      case 14:
        this->intrpt[1].condition = this->registers[arg1 % 64];
        this->programCounter += 2;
        break;
      case 15:
        this->intrpt[1].address = ((this->intrpt[1].address >> 8) << 8) + this->registers[arg1 % 64];
        this->programCounter += 2;
        break;
      case 16:
        this->intrpt[1].address = this->registers[arg1 % 64] * 256 + (this->intrpt[1].address % 256) + ((this->intrpt[1].address >> 16) << 16);
        this->programCounter += 2;
        break;
      case 17:                        
        this->intrpt[1].address = this->intrpt[1].address - ((this->intrpt[1].address >> 16) << 16) + this->registers[arg1 % 64] * 65536;
        this->programCounter += 2;
        break;
      case 18:
        this->intrpt[1].address = this->registers[arg1 % 64] + this->registers[arg2 % 64] * 256 + this->registers[arg3 % 64] * 65536;
        this->programCounter += 4;
        break;
      case 19:
        this->address = ((this->address >> 8) << 8) + this->registers[arg1 % 64];
        this->programCounter += 2;
        break;
      case 20:
        this->address = this->registers[arg1 % 64] * 256 + (this->address % 256) + ((this->address >> 16) << 16);
        this->programCounter += 2;
        break;
      case 21:                        
        this->address = this->address - ((this->address >> 16) << 16) + this->registers[arg1 % 64] * 65536;
        this->programCounter += 2;
        break;
      case 22:
        this->address = this->registers[arg1 % 64] + this->registers[arg2 % 64] * 256 + this->registers[arg3 % 64] * 65536;
        this->programCounter += 4;
        break;
      case 23:
        this->address++;
        this->address %= 16777216;
        this->mem->ovfw = (this->address % 256) == 0;
        this->programCounter += 1;
        break;
      case 24:
        this->address += 256;
        this->address %= 16777216;
        this->mem->ovfw = ((this->address % 65536) - (this->address % 256)) == 0;
        this->programCounter += 1;
        break;
      case 25:
        this->address += 65536;
        this->address %= 16777216;
        this->mem->ovfw = ((this->address) - (this->address % 65536)) == 0;
        this->programCounter += 1;
        break;
      case 26:
        this->address--;
        this->address = positive_modulo(this->address, 16777216);
        this->mem->ovfw = (this->address % 256) == 0;
        this->programCounter += 1;
        break;
      case 27:
        this->address -= 256;
        this->address = positive_modulo(this->address, 16777216);
        this->mem->ovfw = ((this->address % 65536) - (this->address % 256)) == 0;
        this->programCounter += 1;
        break;
      case 28:
        this->address -= 65536;
        this->address = positive_modulo(this->address, 16777216);
        this->mem->ovfw = ((this->address) - (this->address % 65536)) == 0;
        this->programCounter += 1;
        break;
      case 29:
        this->registers[arg1 % 64] = this->mem->read(this->address);
        if (this->mem->autoincrement) {
          this->address++;
          this->address %= 16777216;
          this->mem->ovfw = (this->address % 256) == 0;
        }
        this->programCounter += 2;
        break;
      case 30:
        this->mem->write(this->address, this->registers[arg1 % 64]);
        if (this->mem->autoincrement) {
          this->address++;
          this->address %= 16777216;
          this->mem->ovfw = (this->address % 256) == 0;
        }
        this->programCounter += 2;
        break;
      case 31:
        this->registers[arg1 % 64] = arg2;
        this->programCounter += 3;
        break;
      case 32:
        this->registers[arg1 % 64]++;
        this->mem->ovfw = this->registers[arg1 % 64] == 0;
        this->programCounter += 2;
        break;
      case 33:
        this->registers[arg1 % 64]--;
        this->mem->ovfw = this->registers[arg1 % 64] == 255;
        this->programCounter += 2;
        break;
      case 34:
        this->mem->autoincrement = true;
        this->programCounter += 1;
        break;
      case 35:
        this->mem->autoincrement = false;
        this->programCounter += 1;
        break;
      case 36:
        temp = this->mem->getSizeK();
        this->registers[arg1 % 64] = temp % 256;
        this->registers[arg2 % 64] = temp >> 8;
        this->programCounter += 3;
        break;
      case 37:
        this->registers[arg1 % 64] = this->address % 256;
        this->programCounter += 2;
        break;
      case 38:
        this->registers[arg1 % 64] = (this->address >> 8) % 256;
        this->programCounter += 2;
        break;
      case 39:
        this->registers[arg1 % 64] = (this->address >> 16) % 256;
        this->programCounter += 2;
        break;
      case 40:
        this->registers[arg2 % 64] = this->registers[arg1 % 64];
        this->programCounter += 3;
        break;
      case 41:
        this->registers[arg2 % 64] = this->registers[this->registers[arg1 % 64] % 64];
        this->programCounter += 3;
        break;
      case 42:
        this->registers[arg1 % 64]++;
        this->alu->ovfw = this->registers[arg1 % 64] == 0;
        this->programCounter += 2;
        break;
      case 43:
        this->registers[arg1 % 64]--;
        this->alu->ovfw = this->registers[arg1 % 64] == 255;
        this->programCounter += 2;
        break;
      case 44:
        this->registers[arg3 % 64] = this->alu->ADD(this->registers[arg1 % 64], this->registers[arg2 % 64]);
        this->programCounter += 4;
        break;
      case 45:
        this->registers[arg3 % 64] = this->alu->SUB(this->registers[arg1 % 64], this->registers[arg2 % 64]);
        this->programCounter += 4;
        break;
      case 46:
        this->registers[arg3 % 64] = this->alu->MUL(this->registers[arg1 % 64], this->registers[arg2 % 64]);
        this->programCounter += 4;
        break;
      case 47:
        this->registers[arg3 % 64] = this->alu->MOD(this->registers[arg1 % 64], this->registers[arg2 % 64]);
        this->programCounter += 4;
        break;
      case 48:
        this->registers[arg3 % 64] = this->alu->DIV(this->registers[arg1 % 64], this->registers[arg2 % 64]);
        this->programCounter += 4;
        break;
      case 49:
        this->registers[arg2 % 64] = this->alu->SHL(this->registers[arg1 % 64]);
        this->programCounter += 3;
        break;
      case 50:
        this->registers[arg2 % 64] = this->alu->SHR(this->registers[arg1 % 64]);
        this->programCounter += 3;
        break;
      case 51:
        this->registers[arg3 % 64] = this->alu->AND(this->registers[arg1 % 64], this->registers[arg2 % 64]);
        this->programCounter += 4;
        break;
      case 52:
        this->registers[arg3 % 64] = this->alu->OR(this->registers[arg1 % 64], this->registers[arg2 % 64]);
        this->programCounter += 4;
        break;
      case 53:
        this->registers[arg3 % 64] = this->alu->XOR(this->registers[arg1 % 64], this->registers[arg2 % 64]);
        this->programCounter += 4;
        break;
      case 54:
        this->registers[arg3 % 64] = this->alu->NAND(this->registers[arg1 % 64], this->registers[arg2 % 64]);
        this->programCounter += 4;
        break;
      case 55:
        this->registers[arg3 % 64] = this->alu->NOR(this->registers[arg1 % 64], this->registers[arg2 % 64]);
        this->programCounter += 4;
        break;
      case 56:
        this->registers[arg3 % 64] = this->alu->XNOR(this->registers[arg1 % 64], this->registers[arg2 % 64]);
        this->programCounter += 4;
        break;
      case 57:
        this->alu->SAB(this->registers[arg1 % 64], this->registers[arg2 % 64]);
        this->programCounter += 3;
        break;
      case 58:
        this->registers[arg2 % 64] = this->alu->NOT(this->registers[arg1 % 64]);
        this->programCounter += 3;
        break;
      case 59:

      default:
        SerialSend("ERR","The CPU loaded an unknown instruction, crashing it",50);
        halt();
        break;
    }
  }
}

inline void Emulator::enterInterrupt(bool interrupt) {
  this->inInterrupt = true;
  this->stackmem->pushaddr(this->programCounter);
  this->programCounter = this->intrpt[interrupt].address;
};

inline bool Emulator::cond(uchar cond) {
  switch (cond) {
    case 0:
      return 0;
    case 1:
      return 1;
    case 2:
      return this->alu->A % 2;
    case 3:
      return this->alu->A > 127;
    case 4:
      return this->alu->A > this->alu->B;
    case 5:
      return this->alu->A == this->alu->B;
    case 6:
      return this->alu->A < this->alu->B;
    case 7:
      return this->mem->ovfw;
    case 8:
      return this->alu->ovfw;
    case 9:
      return this->exp->exists();
    case 10:
      return this->exp->hasReply();
    case 11:
      return 1;
  }
};

void loop() {
  EmulatorP emu = new Emulator();
  emu->init();
  emu->run();
}
