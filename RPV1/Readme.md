# SIMA_RPV1
![IMG](/RPV1/img.png)    
ラズベリーパイのRP2350を利用したマイコンボードです。

## SIMAの使い方
WS2182BがGPIO8につながっています
SparkFun_ProMicro_RP2350(RaspiPico2)と互換性があります。  

Bootボタンを押した状態でPCと接続するとブートモードが変更されます。


RP2040(2350)系のボードマネージャーURL
```
https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
```
SparkFun RP2350として認識させて書き込んでください。

LED配列は(ライナー)
```
15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,
31,30....
```
Adafruit_NeoPixelがおすすめ

チップ:RP2350
フラッシュ:16MB

> [!CAUTION]
> 貧弱設計なので無理な使い方をしないでください。

> [!CAUTION]
> 最大輝度でLEDを発光させないでください。

## 発注するには
[HowToOrder.md](./HowToOrder.md)  をご覧ください。

## 制作
**sskrc**

---
