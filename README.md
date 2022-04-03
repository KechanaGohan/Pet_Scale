# Pet_Scale
M5stackCore2を使用したペット体重計

M5stackCore2とGrove-125KHzRFIDリーダー、NAU7802を使用して作成したペット体重計です。   
RFIDタグで個体管理しニフクラ mobile backendにデータが書き込まれます。
RFIDタグを読ませたあとNAU7802モジュールで計測し、M5stackCore2で体重を表示します。計測結果が確定した後、データがニフクラ mobile backendへ送信されます。  
※使用予定だった体重計がうまく動かなかったため、キッチンスケールで代用しています。
## 使用したもの
体重計  
自宅のキッチンスケール

アンプ  
NAU7802  
https://www.switch-science.com/catalog/5539/  

M5stackCore2  
https://www.switch-science.com/catalog/6530/  
ポート拡張のためM5Core2BaseLiteを使用   
https://www.switch-science.com/catalog/6763/  

RFID  
Grove-125KHzRFIDリーダー  
https://wiki.seeedstudio.com/Grove-125KHz_RFID_Reader/  
RFIDタグ  
https://www.sengoku.co.jp/mod/sgk_cart/detail.php?code=EEHD-0RBC  

データ保存先  
ニフクラ mobile backend  
https://mbaas.nifcloud.com/  
※アカウントを作成しておく必要があり  
## 接続図
![pX1IVVizW15NP3r1648985471_1648985498](https://user-images.githubusercontent.com/102903015/161425834-5be7e50b-b287-4d7f-bc35-e03e97f85fba.jpg)
## 使い方
1)電源を入れ、M5stackCore2の画面に"Ready!"と表示されたらペットを乗せます。    
2)ペットが乗っていることを確認したらRFIDを読み取らせます。  
3)測定が完了すると画面に確定した値黄色でが表示されます。  
4)Aボタンを押すと測定した値がリセットされ次の個体測定に移れます。  

Qiitaの記事はこちらです。
