Ghiorghita Alexandra
323CD

server:
 
    Serverul are o conexiune activa in urmatoarele cazuri:
        1. Se primeste o cerere de conexiune de la un client TCP
            - in cazul acesta, se verifica daca exista un client cu ID-ul primit
                - daca nu exista, acesta este adaugat in lista de clienti si se printeaza mesajul specific;
                - daca exista, in caz ca este activ, se ignora cererea, se printeaza mesajul specific
                si se inchide socketul; iar daca este inactiv, se reactiveaza si se trimit toate mesajele
                de la topic-urile la care era abonat cu SF 1.
        2. Se primeste un mesaj de la un client UDP
            - in cazul acesta, parsam mesajul primit si il construim pe cel care trebuie trimis mai
            departe la clientul TCP, in functie de tipul de date;
            - se trimite mesajul creat de noi la clientii activi si abonati la topic sau se stocheaza in lista 
            de delayed_messeges a fiecarui client abonat cu SF 1.
        3. Se primeste mesaj de la STDIN
            - singura comanda care poate fi primita este "exit", iar in acest caz se deconecteaza toti clientii
            activi si se inchid toate coenxiunile.
        4. Se primeste un mesaj de la clientul TCP
            - acest mesaj poate fi de tip "id exit", "id subscribe <topic> <sf>" si "id unsubscribe <topic>"
            - in caz de exit, se deconecteaza clientul care a trimis mesajul (id-ul acestuia se afla
            pe baza socketului) si se inchide socketul;
            - in caz de unsubscribe, se parcurge hashtableul cu topicurile si abonatii la topicul in 
            cauza si se sterge clientul curent din lista de abonati;
            - in caz de subscribe, se creeaza un abonat nou si se verifica daca topicul cerut se afla in 
            lista de topicuri
                - daca da, pur si simplu adaugam clientul in lista de abonati a topic-ului;
                - daca nu, adaugam topicul in hashmap si apoi clientul in lista de abonati.

subscriber:

    Subscriberul are o conexiune activa in urmatoarele cazuri:
        1. Se primeste un mesaj de la STDIN 
            - acesta poate fi de tip "exit", "subscribe <topic> <sf>" si "unsubscribe <topic>";
            - in caz de unsubscribe si subscribe, se concateneaza la mesaj si id-ul clientului;
            - in toate cele trei cazuri, mesajul este trimis mai departe serverului.
        2. Se primeste un mesaj de la server
            - aici este vorba despre mesajele ce privesc abonarilela topicuri (adica de la UDP). In caz de exit,
            se inchide conexiune, altfel, cum mesajul a fost deja formatat la server, el este pur si simplu afisat.