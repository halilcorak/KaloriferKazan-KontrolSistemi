#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>  //  Seri Haberleşme kütüphanesini ekledik
SoftwareSerial BTSerial(10, 11); // RX, TX  // BT için ayrıca bir Seri Haberleşme pini tanımladık

const bool debug = true;

#define fanPin 9 //led bağla
#define pompaPin 8 //led bağla

//Butonların giriş pinleri
#define sag 2
#define sol 3
#define yukari 4
#define asagi 5
#define tamam 6

//Hareket tanımlamaları
#define saga 6
#define sola 4
#define yukariya 8
#define asagiya 2
#define tamama 5
#define hareketYok 0

#define SERIAL_CHECK 0
#define GET_VALUE 1
#define SET_VALUE 2
#define KEY_INDEX 0
#define TYPE_INDEX 1
#define REQUEST_INDEX 2
#define MENU_ID 0
#define MENU_VALUE 1

//Butona basılma durumlarının tutulacağı değişkenler
int sag_durum = 0;
int sol_durum = 0;
int yukari_durum = 0;
int asagi_durum = 0;
int tamam_durum = 0;
int buton_durum = 0;

//Menu Id leri
//Menu ekleme yada değişiklik yapma işine burada başlamanız gerekiyor
//Yeni menü elemanı eklenecek ise verdiğiniz değer aynı olmayacak şekilde aşağıda bir tanımlama yapmalısınız
//Örn: "const int menuYeni=20;
//Daha sonra Setup kısmında kaydını yapmalısınız
//                1          2            3        4    5     6   7   8
//"menuEkle(menuAnaMenu, menuYeni,  "Yeni Menu", true, true, 189, 0, 255);"
//1)Üst menü elemanı, yeni menü elemanı hangi menü elemanının altında görünecek
//2)Yeni oluşturduğumuz tanımlama
//3)Menü elemanının görünecek adı
//4)Altında başka menü elemanları olmayacak ve bir değer tutacaksa (program içerisinde bu değeri kullanabilirisiniz , "menuEl(menuYeni).gecerliDeger" şeklinde erişim sağlayabilirsiniz) "true" değilse "false"
//5)Eproma kayıt edilecek mi?
//6)İlk çalıştığında yada Eprom daki değer sıfır ise bir değer vermek istiyebilirsiniz, programı kullanarak değiştirdiğinizde ARDUINO içerisindeki EPROM a kayıt yapacağı için tanımladığınızı değil EPROM daki değeri getirecektir.
//7)Girilebilecek en düşük değer
//8)Girilebilecek en yüksek değer (Eprom a yapılan kayıt byte türünden olduğu için en yüksek 255 verebilirsiniz)
const byte menuAnaMenu = 1;
const byte menuFan = 2;
const byte menuFanSpeed = 3;
const byte menuFanStopTmp = 4;
const byte menuFanToleranceTmp = 5;
const byte menuPompaTmp = 6;
const byte menuAnahtar = 7;
const byte menuEkranBeklemeSuresi = 8;

//Menuler bu yapıya göre oluşturuluyor
struct menuElemani {
  byte id;
  byte ustId;
  String menu;
  bool degerVar;
  bool eprom;
  byte gecerliDeger;
  byte enFazla;
  byte enDusuk;
};
//Menü elemanlarının tablosu diyebiliriz, yukarıda tanımlanan menü elemanları artıklca buradaki sayıyı da arttırmanız gerekiyor. Örn:"menuElemanlari[30];"
struct menuElemani menuElemanlari[8];

//Ekranda görünen menunun bilgisi, ilk değer "ana menu" Id si
int gosterilenMenuId = 0;//menuAnaMenu;
//Değer değişikliği yapılacak menu bilgisi
byte seciliMenuId = 0;
byte kazanSicakligi = 0;
//Tuşun art arda basma süresinin kontrol etmek ve ana ekrana dönmek için
unsigned long eskiZaman = 0;
bool zamanKontrol = false;

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  pinMode(sag, INPUT);
  pinMode(sol, INPUT);
  pinMode(yukari, INPUT);
  pinMode(asagi, INPUT);
  pinMode(tamam, INPUT);

  pinMode(fanPin, OUTPUT);
  pinMode(pompaPin, OUTPUT);

  Serial.begin(9600);
  Serial.println("Enter AT commands:");
  BTSerial.begin(9600);  //BT Seri haberleşmesini 9600 ile başlattık*

  lcd.init();
  eskiZaman = millis();
  zamanKontrol = true;
  lcd.backlight();

  //Menu yapısı oluşturuluyor, menu elemanlarının tablosu dolduruluyor da diyebiliriz
  menuEkle(0, menuAnaMenu, "Ayarlar", false, false, 0, 0, 0);
  menuEkle(menuAnaMenu, menuFan, "Fan Durumu", true, false, 0, 0, 1);
  menuEkle(menuAnaMenu, menuFanSpeed,  "Fan Hizi", true, true, 0, 0, 255);
  menuEkle(menuAnaMenu, menuFanStopTmp,  "Fan Durma " + String((char)223) + "C", true, true, 50, 40, 80);
  menuEkle(menuAnaMenu, menuFanToleranceTmp, "Fan " + String((char)223) + "C Tlrns", true, true, 15, 5, 20);
  menuEkle(menuAnaMenu, menuPompaTmp,  "Devirdaim " + String((char)223) + "C", true, true, 20, 10, 50);
  menuEkle(menuAnaMenu, menuAnahtar,  "Anahtar", true, true, 67, 0, 255);
  menuEkle(menuAnaMenu, menuEkranBeklemeSuresi,  "Ekran Suresi", true, true, 1, 1, 10);
  //Yeni menü elemanı eklendiğinde buradan kayıt yapmalısınız
  //Örn:"menuEkle(menuAnaMenu, menuYeni,  "Yeni Menu", true, true, 189, 0, 255);"

  EkranOlustur();
}

void loop() {
  sag_durum = digitalRead(sag);
  sol_durum = digitalRead(sol);
  yukari_durum = digitalRead(yukari);
  asagi_durum = digitalRead(asagi);
  tamam_durum = digitalRead(tamam);
  //  Serial.print(sag_durum);
  //  Serial.print(sol_durum);
  //  Serial.print(yukari_durum);
  //  Serial.print(asagi_durum);
  //  Serial.println(tamam_durum);
  KazanKontrol();
  decodeCode();
  DurumDegerlendir();
  unsigned long yeniZaman = millis();
  if (zamanKontrol && yeniZaman - eskiZaman > (menuEl(menuEkranBeklemeSuresi).gecerliDeger * 60000))
  {
    gosterilenMenuId = 0;
    zamanKontrol = false;
    lcd.noBacklight();
    EkranOlustur();
  }
  delay(100);
}

byte mIndex = 0;
void menuEkle(byte uid, byte id,  String m, bool d, bool e, byte gd, byte ed, byte ef)
{
  menuElemanlari[mIndex].id = id;
  menuElemanlari[mIndex].ustId = uid;
  menuElemanlari[mIndex].menu = m;
  menuElemanlari[mIndex].degerVar = d;
  menuElemanlari[mIndex].eprom = e;
  menuElemanlari[mIndex].enFazla = ef;
  menuElemanlari[mIndex].enDusuk = ed;

  //Menü Id si Eprom adresi olarak kullanılıyor
  if (e)
  {
    menuElemanlari[mIndex].gecerliDeger = EEPROM.read(id);
    if (menuElemanlari[mIndex].gecerliDeger == "\n" || menuElemanlari[mIndex].gecerliDeger == 0)
      menuElemanlari[mIndex].gecerliDeger = gd;
    if (menuElemanlari[mIndex].gecerliDeger < ed)
      menuElemanlari[mIndex].gecerliDeger = ed;
    if (menuElemanlari[mIndex].gecerliDeger > ef)
      menuElemanlari[mIndex].gecerliDeger = ef;
  }
  else
    menuElemanlari[mIndex].gecerliDeger = gd;
  mIndex++;
}
//Döngülerde kullanılacak üst değer için
byte menuAdedi()
{
  return mIndex;
}
//Menu Id sine göre array in index ini buluyor
byte bulMenuIndex(byte Id)
{
  for (byte i = 0; i < menuAdedi(); i++)
    if (menuElemanlari[i].id == Id)
      return i;
  return 0;
}

//Ekranı dolduran fonksiyon
void EkranOlustur()
{

  lcd.clear();
  if (gosterilenMenuId == 0)
  {
    lcd.print("Fan   Pompa ISI");
    lcd.setCursor(0, 1);

    if (menuEl(menuFan).gecerliDeger == 1)
      lcd.print("Aktif ");
    else
      lcd.print("Durdu ");

    if (digitalRead(pompaPin) == HIGH)
      lcd.print("Aktif ");
    else
      lcd.print("Durdu ");

    lcd.print(kazanSicakligi);
    lcd.print((char) 223);
    lcd.print("C");
  }
  else
  {
    byte menuIndex = bulMenuIndex(gosterilenMenuId);
    byte ustMenuIndex = 0;
    if (menuElemanlari[menuIndex].ustId != 0)
    {
      ustMenuIndex = bulMenuIndex(menuElemanlari[menuIndex].ustId);
      lcd.print(menuElemanlari[ustMenuIndex].menu);
    }
    if (seciliMenuId != 0)
    {
      lcd.setCursor(15, 0);
      lcd.print("*");
    }
    lcd.setCursor(0, 1);
    lcd.print(menuElemanlari[menuIndex].menu);
    if (altMenusuVarmi(gosterilenMenuId))
      lcd.print(">");
    else if (menuElemanlari[menuIndex].degerVar)
    {
      lcd.setCursor(13, 1);
      if (menuElemanlari[menuIndex].gecerliDeger < 10)
        lcd.print("00");
      else if (menuElemanlari[menuIndex].gecerliDeger < 100)
        lcd.print("0");
      lcd.print(menuElemanlari[menuIndex].gecerliDeger);
    }
  }
}
//Menu elemanının eltında tanımlı menu elemanı varsa true döner
bool altMenusuVarmi(byte menuId)
{
  for (byte i = 0; i < menuAdedi(); i++)
    if (menuElemanlari[i].ustId == menuId)
      return true;
  return false;
}

void KazanKontrol()
{
  //Kazan Sıcaklığı
  int olculenSicaklik = analogRead(A0);
  olculenSicaklik = map(olculenSicaklik, 0, 1023, 0, 100);
  if (olculenSicaklik != kazanSicakligi)
  {
    kazanSicakligi = olculenSicaklik;
    String code = "{\"s\":[" + getJsonCode(0, kazanSicakligi) + "]}";
    BTSerial.print(code);
    delay(200);
    EkranOlustur();
  }

  //Fan Kontrol
  if (menuEl(menuFan).gecerliDeger == 1 && kazanSicakligi >= menuEl(menuFanStopTmp).gecerliDeger)
  {
    byte menuIndex = bulMenuIndex(menuFan);
    menuElemanlari[menuIndex].gecerliDeger = 0;
    analogWrite(fanPin, 0);
    String code = "{\"s\":[" + getJsonCode(menuFan, 0) + "]}";
    BTSerial.print(code);
    EkranOlustur();
  } else if (menuEl(menuFan).gecerliDeger == 0 && kazanSicakligi <= (menuEl(menuFanStopTmp).gecerliDeger - menuEl(menuFanToleranceTmp).gecerliDeger) )
  {
    byte menuIndex = bulMenuIndex(menuFan);
    menuElemanlari[menuIndex].gecerliDeger = 1;
    analogWrite(fanPin, menuEl(menuFanSpeed).gecerliDeger);
    String code = "{\"s\":[" + getJsonCode(menuFan, 1) + "]}";
    BTSerial.print(code);
    EkranOlustur();
  }
  //Devirdaim Kontrol
  if (digitalRead(pompaPin) == LOW && kazanSicakligi >= menuEl(menuPompaTmp).gecerliDeger)
  {
    digitalWrite(pompaPin, HIGH);
    EkranOlustur();
  } else if (digitalRead(pompaPin) == HIGH && kazanSicakligi < menuEl(menuPompaTmp).gecerliDeger  )
  {
    digitalWrite(pompaPin, LOW);
    EkranOlustur();
  }
}

//Menu Id ye göre menu elemanı döner
struct menuElemani menuEl(byte id)
{
  int menuIndex = bulMenuIndex(id);
  return menuElemanlari[menuIndex];
}

//Yukarı tuşuna basıldığında gösterilecek menü yü ayarlar
void menuYukari()
{
  int menuIndex = bulMenuIndex(gosterilenMenuId);
  if (menuElemanlari[menuIndex].ustId != 0)
  {
    byte m[menuAdedi()];
    byte mIx = 0;
    byte umId = menuElemanlari[menuIndex].ustId;
    byte gI = 0;
    for (byte i = 0; i < menuAdedi(); i++)
    {
      if (menuElemanlari[i].ustId == umId)
      {
        m[mIx] = i;
        if (menuIndex == i)
          gI = mIx;
        mIx++;
      }
    }
    if (gI == 0)
      gI = mIx - 1;
    else
      gI--;
    gosterilenMenuId = menuElemanlari[m[gI]].id;
  }
}
//Aşağı tuşuna basıldığında gösterilecek menüyü ayarlar
void menuAsagi()
{
  byte menuIndex = bulMenuIndex(gosterilenMenuId);
  if (menuElemanlari[menuIndex].ustId != 0)
  {
    byte m[menuAdedi()];
    byte mIx = 0;
    byte umId = menuElemanlari[menuIndex].ustId;
    byte gI = 0;
    for (byte i = 0; i < menuAdedi(); i++)
    {
      if (menuElemanlari[i].ustId == umId)
      {
        m[mIx] = i;
        if (menuIndex == i)
          gI = mIx;
        mIx++;
      }
    }
    if ((gI + 1) == mIx)
      gI = 0;
    else
      gI++;
    gosterilenMenuId = menuElemanlari[m[gI]].id;
  }
}
//Alt menü elemanları varsa ilkini görüntülemek için ayarlar
void menuSag()
{
  for (byte i = 0; i < menuAdedi(); i++)
  {
    if (menuElemanlari[i].ustId == gosterilenMenuId)
    {
      gosterilenMenuId = menuElemanlari[i].id;
      return;
    }
  }
}
//Bir üst menü elemanına döndürür
void menuSol()
{
  int menuIndex = bulMenuIndex(gosterilenMenuId);
  //if (menuElemanlari[menuIndex].ustId != 0)
  gosterilenMenuId = menuElemanlari[menuIndex].ustId;
}

//Basılan tuşa göre yapılacak işlem ayarlanır
void DurumDegerlendir()
{
  byte yeni_buton_durum = YeniButonDurumGetir();
  unsigned long yeniZaman = millis();
  //Herhangi bir tuşa basıldıysa
  if (yeniZaman - eskiZaman > 300 && yeni_buton_durum != hareketYok) //yeni_buton_durum != buton_durum
  {
    eskiZaman = millis();
    buton_durum = yeni_buton_durum;
    zamanKontrol = true;
    lcd.backlight();
    switch (buton_durum)
    {
      case saga://sag
        {
          if (seciliMenuId == 0)
            menuSag();
          else
            DegerDegistir(1);
          EkranOlustur();
          break;
        }
      case sola://sol
        {
          if (seciliMenuId == 0)
            menuSol();
          else
            DegerDegistir(-1);
          EkranOlustur();
          break;
        }
      case yukariya://yukarı
        {
          if (seciliMenuId == 0)
            menuYukari();
          else
            DegerDegistir(10);
          EkranOlustur();
          break;
        }
      case asagiya://asağı
        {
          if (seciliMenuId == 0)
            menuAsagi();
          else
            DegerDegistir(-10);
          EkranOlustur();
          break;
        }
      case tamama://tamam
        {
          if (seciliMenuId == 0 && menuEl(gosterilenMenuId).degerVar)
          {
            seciliMenuId = gosterilenMenuId;
          }
          else if (seciliMenuId != 0)
          {
            byte menuIndex = bulMenuIndex(seciliMenuId);
            if (menuElemanlari[menuIndex].eprom)
              EEPROM.write(seciliMenuId, menuElemanlari[menuIndex].gecerliDeger);

            FanKontrol(seciliMenuId);

            String code = "";
            code = getJsonCode(seciliMenuId, menuElemanlari[menuIndex].gecerliDeger);
            code = "{\"s\":[" + code + "]}";
            BTSerial.print(code);

            seciliMenuId = 0;

          }
          EkranOlustur();
          break;
        }
    }
  }
}
//Menü elemanının değeri istenilen değer eklenerek yada çıkartılarak ayarlanır
void DegerDegistir(byte stp)
{
  int menuIndex = bulMenuIndex(seciliMenuId);
  menuElemanlari[menuIndex].gecerliDeger += stp;
  if (menuElemanlari[menuIndex].gecerliDeger < menuElemanlari[menuIndex].enDusuk)
    menuElemanlari[menuIndex].gecerliDeger = menuElemanlari[menuIndex].enDusuk;
  if (menuElemanlari[menuIndex].gecerliDeger > menuElemanlari[menuIndex].enFazla)
    menuElemanlari[menuIndex].gecerliDeger = menuElemanlari[menuIndex].enFazla;

}
//Hangi butona göre işlem yapılacağı belirlenir
byte YeniButonDurumGetir()
{
  byte yeni_buton_durum = hareketYok;
  if (sag_durum == 1 && sol_durum == 0 && yukari_durum == 0 && asagi_durum == 0 && tamam_durum == 0)
  {
    yeni_buton_durum = saga;
  }
  else if (sag_durum == 0 && sol_durum == 1 && yukari_durum == 0 && asagi_durum == 0 && tamam_durum == 0)
  {
    yeni_buton_durum = sola;
  }
  else if (sag_durum == 0 && sol_durum == 0 && yukari_durum == 1 && asagi_durum == 0 && tamam_durum == 0)
  {
    yeni_buton_durum = yukariya;
  }
  else if (sag_durum == 0 && sol_durum == 0 && yukari_durum == 0 && asagi_durum == 1 && tamam_durum == 0)
  {
    yeni_buton_durum = asagiya;
  }
  else if (sag_durum == 0 && sol_durum == 0 && yukari_durum == 0 && asagi_durum == 0 && tamam_durum == 1)
  {
    yeni_buton_durum = tamama;
  }
  else
  {
    yeni_buton_durum = hareketYok;
  }

  return yeni_buton_durum;
}

void decodeCode()
{
  String code = getCode();
  if (code != "")
  {
    int requestCode = getCodeValue(code, ';', TYPE_INDEX).toInt();
    switch (requestCode)
    {
      case   SERIAL_CHECK:
        {
          BTSerial.write("OK");
          break;
        }
      case   GET_VALUE:
        {
          String code = "";
          code = "{\"s\":[" + getJsonCode(0, kazanSicakligi) + "]}";
          BTSerial.print(code);
          Serial.println(code);
          delay(200);
          for (int i = 6; i > 1; i--)
          {
            int menuIndex = bulMenuIndex(i);
            code = getJsonCode(i, menuElemanlari[menuIndex].gecerliDeger);
            code = "{\"s\":[" + code + "]}";
            BTSerial.print(code);
            Serial.println(code);
            delay(200);
          }
          break;
        }
      case   SET_VALUE:
        {
          setMenuValue(getCodeValue(code, ';', REQUEST_INDEX));
          break;
        }
      default:
        {
          break;
        }
    }
  }
}

String getJsonCode(byte o, byte v)
{
  return "{\"o\":\"" + String(o) + "\",\"v\":\"" + String(v) + "\"}";
}

void setMenuValue(String code)
{
  if (code == "") return;

  byte menuId = getCodeValue(code, ',', MENU_ID).toInt();
  byte menuValue = getCodeValue(code, ',', MENU_VALUE).toInt();

  byte menuIndex = bulMenuIndex(menuId);
  if (menuElemanlari[menuIndex].gecerliDeger != menuValue && menuValue >= menuElemanlari[menuIndex].enDusuk && menuValue <= menuElemanlari[menuIndex].enFazla)
  {
    menuElemanlari[menuIndex].gecerliDeger = menuValue;
    if (menuElemanlari[menuIndex].gecerliDeger < menuElemanlari[menuIndex].enDusuk)
      menuElemanlari[menuIndex].gecerliDeger = menuElemanlari[menuIndex].enDusuk;
    if (menuElemanlari[menuIndex].gecerliDeger > menuElemanlari[menuIndex].enFazla)
      menuElemanlari[menuIndex].gecerliDeger = menuElemanlari[menuIndex].enFazla;
    if (menuElemanlari[menuIndex].eprom)
      EEPROM.write(menuId, menuElemanlari[menuIndex].gecerliDeger);

    FanKontrol(menuId);

    EkranOlustur();
  }
  Serial.print("Menu ID:");
  Serial.println(menuId);
  Serial.print("Value:");
  Serial.println(menuValue);
}

void FanKontrol(byte id)
{
  if (id == menuFan)
  {
    if (menuEl(menuFan).gecerliDeger == 1)
      analogWrite(fanPin, menuEl(menuFanSpeed).gecerliDeger);
    else
      analogWrite(fanPin, 0);
  }
  if (id == menuFanSpeed)
  {
    if (menuEl(menuFan).gecerliDeger == 1)
      analogWrite(fanPin, menuEl(menuFanSpeed).gecerliDeger);
  }
}

String getCode()
{
  String code = "";
  bool beginGet = false;
  char c ;
  while (BTSerial.available())
  {

    c = BTSerial.read();
    code += c;
    delay(10);
  }

  if (code != "")
    if (debug)
      Serial.println(code);

  byte startIndex = code.indexOf("(");
  byte endIndex = code.indexOf(")");
  if (startIndex != -1 && endIndex != -1 && endIndex > startIndex)
  {
    code = code.substring(startIndex + 1, endIndex);
  }
  if (!checkKey(code))
    code = "";


  return code;
}

bool checkKey(String code)
{
  int menuIndex = bulMenuIndex(menuAnahtar);
  String keyFormat = "#" + String(menuElemanlari[menuIndex].gecerliDeger) + "#";
  if (code.indexOf(keyFormat) == 0)
    return true;
  else
    return false;
}

String getCodeValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
