# garagentor-esp8266
Garagentor-Steuerung mit ESP8266 und Arduino via http und mqtt für openHAB - Hörmann UAP1

Hardware-Aufbau und Idee von http://www.majorshark.de/index.php/13-hoermann-torantrieb-mit-espeasy-in-fhem-einbinden
Frank's Technik Blog	
- statt FHEM und ESPeasy aber openHAB und Arduino-Programmierung

--

Wenn man ein Hörmann Garagentor mit SupraMatic Antrieb in openHAB einbinden und bedienen möchte, benötigt man eine Erweiterung des Antriebes. Mit der Hörmann Universaladapterplatine UAP 1 erweitert man den Hörmann Antrieb um die benötigten Ein- und Ausgänge. Damit ist es dann möglich, das Tor mit einem Knopfdruck in die entsprechende Richtung fahren zu lassen und - was noch viel wichtiger ist - auch zu sehen, ob das Tor gerade geöffnet oder geschlossen ist.

Die Ausgänge sind als potentialfreie Relaiskontakte ausgeführt. Die Eingänge werden einfach mit einem 1 sec-Impuls auf Masse gezogen. Im einzelnen bietet die Universaladapterplatine UAP1 folgende Ein- und Ausgänge:

Eingang 	Klemme
Tor öffnen 	S2 	
Tor schließen 	S4 	
Tor Teilöffnung 	S3 	
Antriebsbeleuchtung toggeln 	S5 	  	 
Antrieb ausschalten 	S0

Ausgang 	Klemme
Endlagenmeldung Tor offen 	O1
Endlagenmeldung Tor zu 	O2
Entspricht der Programierung des Optionsrelais (Blinklicht, Beleuchtung etc.) 	O3


Weiterhin befindet sich ein 24V Anschluss auf der UAP 1, der mit 100mA belastet werden kann. Außerdem ein weiterer Anschluss, um das Tor komplett außer Betrieb zu nehmen. Dieser ist aber ab Werk mit einer Drahtbrücke versehen.

BOM

1x 	Hörmann Universaladapterplatine UAP 1 	ca. 50,00€
1x 	Wemos D1 	ca. 7,00€
1x 	DC-DC Step Down Power Supply Module 	ca. 1,70€ LM2596S
4x  T1-T4 NPN BC546B
4x  1uF
4x  15k
4x  2,2k
4x  2,0k
4x  LED
1x  100 uF
Universalplatine, Schaltdraht, Lötkolben


Schaltungsaufbau:

Im wesentlichen besteht die Schaltung aus einem Wemos D1 und ein paar Bauteilen aus der Grabbelkiste. Die beiden Ausgänge der UAP 1 für Tor Endlage offen und Tor Endlage geschlossen sind als Relaisausgänge ausgeführt. Sie können direkt an den Wemos D1 angeschlossen werden. Die Entprellung der Relais-kontakte übernimmt dabei die Software (ESPEasy) auf dem ESP8266. Mit dem Relaisausgang O3 habe ich das jetzt weggefallene Hörmann Optionsrelais HOR1 für das Licht ersetzt.

Um die vier Eingänge der UAP 1 ansteuern zu können, benötigt man noch vier einfache Schalttransistoren. Diese Transistoren können mit den 3,3V Signalen des ESP die 24V der UAP 1 schalten. Der Blogger Otto gibt auf seinem Blog einen Strom von 4,8mA an, wenn die Eingänge der UAP 1 auf Masse gezogen werden. Die Spannung an den offenen Eingängen gibt er mit 21V an. Da bei mir noch Unmengen an DDR Transistoren in den Kisten liegen habe ich mich für einen SC237 npn-Tansistor entschieden. Der Vergleichstyp ist heute der BC237. Dieser kann laut Datenblatt Spannungen bis zu 45V und Ströme bis zu 100mA schalten. Also völlig ausreichend für diesen Einsatzzweck. Die Pull-Down Widerstände (R1,R4,R7,R10) bringen Ordnung in das etwas zickige Bootverhalten der GPIO des ESP. Ohne diese Widerstände werden die GPIO beim Booten des ESP einmal kurz auf High gezogen. Das ist natürlich unbedingt zu Vermeiden. Die Tiefpässe sollen weitere Störungen verhindern die zu einem ungewollten Schalten der Ausgänge führen können.

Der Spannungswandler ist ebenfalls ein "Cent" Artikel aus dem Onlineshop. Fünf Stück für derzeit 8,50 €. Dieser wandelt nach dem Datenblatt Spannungen zwischen 4,5V bis 28V auf 0,8V bis 20V bei einem Dauerstrom von 3A um. Die 24V der UAP1 werden mit diesem Wandler nahezu Verlustfrei auf 3,3V gewandelt. 

ESP 	Wemos D1 	Funktion 	UAP1 	Funktion UAP1 
GPIO5 	D1 	Eingang 	O1 	Tor Endlage oben (Geöffnet)
GPIO0 	D3 	Eingang 	O2 	Tor Endlager unten (Geschlossen)
GPIO14 	D5 	Ausgang 	S4 	Tor schließen
GPIO12 	D6 	Ausgang 	S5 	Antriebsbeleuchtung toggeln
GPIO13 	D7 	Ausgang 	S2 	Tor öffnen
GPIO15 	D8 	Ausgang 	S3 	Tor Teilöffnung 

Zusammengefasst in einem Schaltbild sieht es dann so aus. Links die Relaisausgänge der UAP 1, rechts die Eingänge inkl. der Stromversorgung von der UAP 1.

Aufbau auf der Universalplatine 

Aufgebaut habe ich das Ganze auf einer universal Lochrasterplatine. Die Platine hat die Maße 9x7cm. Damit die Platine in das Kästchen der UAP1 passt, muss diese noch zurecht geschnitten werden. In dem UAP1-Kasten sind seitlich kleine Stege. Diese habe ich in der Platine ausgeschnitten.

