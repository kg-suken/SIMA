# SIMA_RPV1
![IMG](/RPV1/img.png)    
ラズベリーパイのRP2350を利用したマイコンボードです。

## SIMAの使い方
WS2182BがGPIO8につながっています
SparkFun_ProMicro_RP2350(RaspiPico2)と互換性があります。

ボードマネージャーURL
```
https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
```

LED配列は(ライナー)
```
15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,
31,30....
```
Adafruit_NeoPixelがおすすめ

チップ:RP2350
フラッシュ:16MB

貧弱設計なので無理な使い方をしないでください。

## 制作
**sskrc**

---
