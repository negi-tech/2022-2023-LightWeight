# 2022-2023-LightWeight
コーディング規則
　１．変数、配列の命名など
 　　日本語ではなく英単語１文字でなるべく表記する。単語同士を _ で繋いだ長い名前は極力避けること。定数は#defineで定義し、全て大文字にする。配列はconst~で定義し同じくすべて大文字。配列の 　　要素数も定数(or 変数)で表す。変数はすべて小文字で表記。グローバル変数の宣言は極力減らす。
   
  ２．クラスについて
  　　なるべくモジュールごとにクラスを用意する。(例 モーター、ジャイロ、UI...)プログラムの中でバラバラに関数、変数が存在しているといろいろめんどくさいのでまとめてクラスにして包括して上げる。publicの変数、関数は最小にしてなるべくprivateとする。classの定義はmain.cppでは行わず別に○○○.hを用意し、そこに記述していく。


概要
高崎高校物理部のロボカップジュニア ライトウェイト部門のリポジトリ
今後の予定：
　メインマイコン：Teensy 4.0
  赤外線センサー：tssp58038 x16 + Seeeduino XIAO RP2040 x1
  ラインセンサ：NJL7502l x32 + Seeeduino XIAO RP2040 x2
  モーター：Pololu 20.4:1 HP 12V x4 (Maxon買ったけど届かない)
  モータードライバ：Pololu G2 18v17 x4
  キッカー：CB1037
  オムニホイール：4輪 自作
  カメラ：Open MV M7 R2
  電源：Li-Poバッテリー 11.1V 3cell 1000mA

チームメンバー
　ハード担当 根岸 孝次 (リーダー) 
　ソフト担当 斎藤 孝介

連絡先:koji.personal@gmail.com
