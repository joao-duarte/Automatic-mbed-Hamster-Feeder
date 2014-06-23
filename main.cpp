/////////////////////////////////////////////Alimentador Automatico////////////////////////////////////////////////////
// Trabalho realizado por:
//
// Fabio Guerreiro  n.35538
// Joao Duarte      n.36556

/////////////////////////////////////////////////// Includes: /////////////////////////////////////////////////////////
#include "mbed.h"     
#include "EthernetNetIf.h"
#include "NTPClient.h"
#include "SMTPClient.h"
#include "HTTPServer.h"
#include "HTTPClient.h"

////////////////////////////////////////////////// Definicoes Email: ///////////////////////////////////////////////////
#define HOSTNAME "mbedSE"
#define SERVER "smtp.ualg.pt" //this definitions should be changed
#define PORT 25
#define USER "******"         //Sorry cant show you this - ualg username aXXXXX
#define PASSWORD "*******"    //or this 
#define DOMAIN "mailnull.com"
#define FROM_ADDRESS "gaiola_de_gerbos@gmail.com"

//////////////////////////////////////////////// Entradas e Saidas: ////////////////////////////////////////////////////
DigitalIn foodsense(p12,"comida"), highwater(p13,"aguacheia"), lowwater(p14,"aguavazia"), conf(p5);
DigitalOut motor(p16,"motor"), valve(p17,"electrovalvula"), fan(p6,"ventoinha"), heater(p7,"aquecedor");
DigitalOut led1(LED1)/*falta de comida*/, led2(LED2)/*falta de agua*/, led3(LED3)/*temperatura baixa*/, led4(LED4)/*temperatura alta*/;         
DigitalOut rst(p30);
DigitalOut clk(p29);
DigitalInOut dq(p28);

//////////////////////////////////////////////// Comunicacao Serie: ///////////////////////////////////////////////////////
Serial pc(USBTX, USBRX);
Serial gsm(p9,p10);

/////////////////////////////////////////////// Definicoes da Internet ////////////////////////////////////////////////////
LocalFileSystem local("local");

Host host(IpAddr(), PORT, SERVER);                                  //Definicoes para
SMTPClient smtp(host, DOMAIN, USER, PASSWORD, SMTP_AUTH_PLAIN);     //protocolo SMTP

HTTPServer svr;     //Definicoes para criar pagina web
HTTPClient http;    //Definicoes para enviar dados do grafico

///////////////////////////////////////////////// Variaveis Globais: ///////////////////////////////////////////////////////

int food, water, temperature;
int count1, count2, count3, countemail;
float tmin=10,tmax=40, temp;    // valores standard de temperatura
char ncm[14],nt[10],msg1[141],msg2[141],msg3[141],email[256],tempmin[5],tempmax[5];

//////////////////////////////////////////// Definicoes da API ThingsSpeak: ////////////////////////////////////////////////
char* thingSpeakUrl = "http://api.thingspeak.com/update";
char* thingSpeakKey = "*************"; //Sorry, cant show this either - API Key
char urlBuffer[256];

///////////////////////////////////////////////// Programas Auxiliares: /////////////////////////////////////////////////////

void intro(int inout)   //Sequencia de leds para configuracao
{
    led1=0;led2=0;led3=0;led4=0;
    wait(0.3);
    for(int k=0;k<2;k++)
    {
        wait(0.1);
        if(inout==1)
        {
        led1=1;wait(0.1);led1=0;
        led2=1;wait(0.1);led2=0;
        led3=1;wait(0.1);led3=0;
        led4=1;wait(0.1);led4=0;
        }
        else
        {
        led4=1;wait(0.1);led4=0;
        led3=1;wait(0.1);led3=0;
        led2=1;wait(0.1);led2=0;
        led1=1;wait(0.1);led1=0;
        }
    }
    wait(0.3);
}

void gravar()   //Grava dados da configuracao num ficheiro
{
    FILE *fp = fopen("/local/config.txt", "w");  
    fprintf(fp, "%s",ncm);
    fprintf(fp, "%s",nt);
    fprintf(fp, "%d",countemail);
    fprintf(fp, "%s",email);
    fprintf(fp, "%d",count1);
    fprintf(fp, "%s",msg1);
    fprintf(fp, "%d",count2);
    fprintf(fp, "%s",msg2);
    fprintf(fp, "%d",count3);
    fprintf(fp, "%s",msg3);
    fprintf(fp, "%.1f ",tmin);
    fprintf(fp, "%.1f ",tmax);
    fclose(fp);
}

int ler()   //Le os dados do ficheiro de configuracao
{
    FILE *fp;
    if(!(fp=fopen("/local/config.txt","r")))
    {
        pc.printf("\n\rERRO! O ficheiro de configuracao nao foi encontrado!");
        pc.printf("\n\rConfigure os parametros do sistema.");
        pc.printf("\n\nPrima qualquer tecla para continuar...\n\r");
        pc.getc();
        return(1);
    }
    else
    {
        fgets(ncm,14,fp);               
        fgets(nt,10,fp);               
        fscanf(fp,"%d",&countemail);   
        fgets(email,countemail,fp);    
        fscanf(fp,"%d",&count1);       
        fgets(msg1,count1,fp);         
        fscanf(fp,"%d",&count2);       
        fgets(msg2,count2,fp);         
        fscanf(fp,"%d",&count3);       
        fgets(msg3,count3,fp);         
        fscanf(fp,"%f",&tmin);         
        fscanf(fp,"%f",&tmax);         
        fclose(fp);
        return(0);
    }
}

void sms (char msg[141])    //Envia SMS
{
    ler();
    gsm.printf("AT+CMGF=1");
    gsm.putc(13); //CR
    wait(0.5);
    gsm.printf("AT+CSCA=\"%s\"",ncm);
    gsm.putc(13); //CR
    wait(0.5);
    gsm.printf("AT+CMGS=\"%s\"",nt);
    gsm.putc(13); //CR
    wait(0.5);
    gsm.printf("%s",msg);
    gsm.putc(26); //Ctrl+Z
    pc.printf("\n\rsms enviado: \"%s\"",msg);
}

void clrscr()   //Limpa o ecra do programa terminal
{
    pc.putc(0x1B);
    pc.printf("[2J");
    pc.putc(12); 
}

void config(int erro)   //Modo de configuracao
{
    char key, key2;
    int i, ascii, mod;
    clrscr();
    intro(1);
    if(erro==0)
        ler();
    led1=1;led2=1;led3=1;led4=1;
    mod=0;
    do{
        clrscr();
        pc.printf("\r+++++ CONFIGURACAO DO SISTEMA +++++\n\n\r");
        pc.printf("[1] Numero do Centro de Mensagens: %s\n\n\r",ncm);
        pc.printf("[2] Numero de Telefone: %s\n\n\r",nt);
        pc.printf("[3] Endereco Electronico: %s\n\n\r",email);
        pc.printf("[4] Mensagem de Temperatura Fora de Limites: %s\n\n\r",msg1);
        pc.printf("[5] Mensagem de Falta de Agua: %s\n\n\r",msg2);
        pc.printf("[6] Mensagem de Falta de Comida: %s\n\n\r",msg3);
        pc.printf("[7] Enviar Mensagem de Teste\n\n\r");
        pc.printf("[8] Temperatura Minima: %.1f C\n\n\r",tmin);
        pc.printf("[9] Temperatura Maxima: %.1f C\n\n\r",tmax);
        pc.printf("[0] Sair do Menu de Configuracao\n\n\r");
        pc.printf("Seleccione uma opcao:");
        key=pc.getc();
        pc.printf("%c",key);
        pc.getc();
        switch(key) {
    
            case '1' :  mod=1;
                        clrscr();
                        pc.printf("\rIntroduza o numero do Centro de Mensagens: ");
                        for(i=0;i<14;i++)
                        {                                   //faz mais 1 iteracao que necessario para possibilidade de corrigir ultimo digito
                            ncm[i]=pc.getc();               //guarda o numero de telemovel, caracter a caracter
                            ascii=ncm[i];                   //guarda o codigo ascii decimal desse caracter
                            if(i<13 || ascii==127)          //se nao estiver na ultima iteracao ou em caso de backspace mostra o caracter escolhido
                                pc.printf("%c",ncm[i]);     
                            if(ascii==127)                  //se o caracter for um backspace volta um ciclo atras
                                i=i-2;
                            if(i==13)                       //se estiver na ultima iteracao mete um espaco no fim da string
                                ncm[i]='\0';                 
                        }
                        break;
                        
            case '2' :  mod=1;
                        clrscr();
                        pc.printf("\rIntroduza o numero de Telefone: ");
                        for(i=0;i<10;i++)
                        {                                   //faz mais 1 iteracao que necessario para possibilidade de corrigir ultimo digito
                            nt[i]=pc.getc();                //guarda o numero de telemovel, caracter a caracter
                            ascii=nt[i];                    //guarda o codigo ascii decimal desse caracter
                            if(i<10 || ascii==127)          //se nao estiver na ultima iteracao ou em caso de backspace mostra o caracter escolhido
                                pc.printf("%c",nt[i]);     
                            if(ascii==127)                  //se o caracter for um backspace volta um ciclo atras
                                i=i-2;
                            if(i==9)                        //se estiver na ultima iteracao mete um espaco no fim da string
                                nt[i]='\0';                 
                        }
                        break;
                    
            case '3' :  mod=1;
                        clrscr();
                        pc.printf("\rIntroduza o Endereco Electronico: ");
                        for(i=0;i<256;i++)
                        {
                            email[i]='\0';                    //limpar o array
                        }
                        for(i=0;i<256;i++)
                        {                                     //faz mais 1 iteracao que necessario para possibilidade de corrigir ultimo digito
                            email[i]=pc.getc();               //guarda o endereco, caracter a caracter
                            ascii=email[i];                   //guarda o codigo ascii decimal desse caracter
                            if(i<256 || ascii==127)           //se nao estiver na ultima iteracao ou em caso de backspace mostra o caracter escolhido
                                pc.printf("%c",email[i]);     
                            if(ascii==127)                    //se o caracter for um backspace volta um ciclo atras
                                i=i-2;
                            if(i==255 || ascii==13)           //se estiver na ultima iteracao ou se receber enter mete um espaco no fim da string
                            {
                                email[i]='\0';
                                countemail=i+1;
                                i=255;
                            }        
                        }
                        break;        
                                           
            case '4' :  mod=1;
                        clrscr();
                        pc.printf("\rIntroduza a mensagem: ");
                        for(i=0;i<141;i++)
                        {
                            msg1[i]='\0';                     //limpar o array
                        }
                        for(i=0;i<141;i++)
                        {                                     //faz mais 1 iteracao que necessario para possibilidade de corrigir ultimo digito
                            msg1[i]=pc.getc();                //guarda a mensagem, caracter a caracter
                            ascii=msg1[i];                    //guarda o codigo ascii decimal desse caracter
                            if(i<141 || ascii==127)           //se nao estiver na ultima iteracao ou em caso de backspace mostra o caracter escolhido
                                pc.printf("%c",msg1[i]);     
                            if(ascii==127)                    //se o caracter for um backspace volta um ciclo atras
                                i=i-2;
                            if(i==140 || ascii==13)           //se estiver na ultima iteracao ou se receber enter mete um espaco no fim da string
                            {
                                msg1[i]='\0';
                                count1=i+1;
                                i=140;
                            }        
                        }
                        break;
            
            case '5' :  mod=1;
                        clrscr();
                        pc.printf("\rIntroduza a mensagem: ");
                        for(i=0;i<141;i++)
                        {
                            msg2[i]='\0';                     //limpar o array
                        }
                        for(i=0;i<141;i++)
                        {                                     //faz mais 1 iteracao que necessario para possibilidade de corrigir ultimo digito
                            msg2[i]=pc.getc();                //guarda a mensagem, caracter a caracter
                            ascii=msg2[i];                    //guarda o codigo ascii decimal desse caracter
                            if(i<141 || ascii==127)           //se nao estiver na ultima iteracao ou em caso de backspace mostra o caracter escolhido
                                pc.printf("%c",msg2[i]);     
                            if(ascii==127)                    //se o caracter for um backspace volta um ciclo atras
                                i=i-2;
                            if(i==140 || ascii==13)           //se estiver na ultima iteracao ou se receber enter mete um espaco no fim da string
                            {
                                msg2[i]='\0';
                                count2=i+1;
                                i=140;
                            }        
                        }
                        break;
            
            case '6' :  mod=1;
                        clrscr();
                        pc.printf("\rIntroduza a mensagem: ");
                        for(i=0;i<141;i++)
                        {
                            msg3[i]='\0';                     //limpar o array
                        }
                        for(i=0;i<141;i++)
                        {                                     //faz mais 1 iteracao que necessario para possibilidade de corrigir ultimo digito
                            msg3[i]=pc.getc();                //guarda a mensagem, caracter a caracter
                            ascii=msg3[i];                    //guarda o codigo ascii decimal desse caracter
                            if(i<141 || ascii==127)           //se nao estiver na ultima iteracao ou em caso de backspace mostra o caracter escolhido
                                pc.printf("%c",msg3[i]);     
                            if(ascii==127)                    //se o caracter for um backspace volta um ciclo atras
                                i=i-2;
                            if(i==140 || ascii==13)           //se estiver na ultima iteracao ou se receber enter mete um espaco no fim da string
                            {
                                msg3[i]='\0';
                                count3=i+1;
                                i=140;
                            }        
                        }
                        break;

            case '7' :  clrscr();
                        pc.printf("Pretende enviar uma mensagem de teste? [S]im ou [N]ao\n\n\r");
                        do
                        {
                            key2=pc.getc();
                            pc.printf("%c",key2);
                        }while(key2!='s' && key2!='S' && key2!='n' && key2!='N');
                        pc.getc();
                        clrscr();
                        if(key2=='s' || key2=='S')
                        {
                            sms("Mensagem de Teste!");
                            wait(2);
                        }
                        break;
                        
            case '8' :  mod=1;
                        clrscr();
                        pc.printf("\rIntroduza a Temperatura Minima: ");
                        for(i=0;i<5;i++)
                        {
                            tempmin[i]='\0';                     //limpar o array
                        }
                        for(i=0;i<5;i++)
                        {                                        //faz mais 1 iteracao que necessario para possibilidade de corrigir ultimo digito
                            tempmin[i]=pc.getc();                //guarda o valor da temperatura minima, caracter a caracter
                            ascii=tempmin[i];                    //guarda o codigo ascii decimal desse caracter
                            if(i<5 || ascii==127)                //se nao estiver na ultima iteracao ou em caso de backspace mostra o caracter escolhido
                                pc.printf("%c",tempmin[i]);     
                            if(ascii==127)                       //se o caracter for um backspace volta um ciclo atras
                                i=i-2;
                            if(i==4 || ascii==13)                //se estiver na ultima iteracao ou se receber enter mete um espaco no fim da string
                            {
                                tempmin[i]='\0';
                                i=4;
                            }        
                        }
                        tmin=atof(tempmin);                      //converte a string para float
                        break;
                    
            case '9' :  mod=1;
                        clrscr();
                        pc.printf("\rIntroduza a Temperatura Maxima: ");
                        for(i=0;i<5;i++)
                        {
                            tempmax[i]='\0';                     //limpar o array
                        }
                        for(i=0;i<5;i++)
                        {                                        //faz mais 1 iteracao que necessario para possibilidade de corrigir ultimo digito
                            tempmax[i]=pc.getc();                //guarda o valor da temperatura maxima, caracter a caracter
                            ascii=tempmax[i];                    //guarda o codigo ascii decimal desse caracter
                            if(i<5 || ascii==127)                //se nao estiver na ultima iteracao ou em caso de backspace mostra o caracter escolhido
                                pc.printf("%c",tempmax[i]);     
                            if(ascii==127)                       //se o caracter for um backspace volta um ciclo atras
                                i=i-2;
                            if(i==4 || ascii==13)                //se estiver na ultima iteracao ou se receber enter mete um espaco no fim da string
                            {
                                tempmax[i]='\0';
                                i=4;
                            }        
                        }
                        tmax=atof(tempmax);                      //Converte a string para float
                       break;
        }
    }while(key!='0');
    clrscr(); 
    pc.printf("Configuracao Terminada!\n\n\rSistema em Funcionamento...\n\r");
    if(mod==1)    //Se houver modificacoes, grava o ficheiro
        gravar(); 
    intro(0);
}

void send_temp (long command)   //Envia comandos para o sensor de temperatura DS1620
{
    dq.output();
    rst=0;
    wait_us(5);
    rst=1;
    for(int i=0 ; i<8 ; i++)
    {
        clk=0;
        wait_us(5);
        dq=command & (1<<i);
        wait_us(5);
        clk=1;
        wait_us(5);
    }
}

float read_temp ()  //Le a temperatura dada pelo o sensor DS1620
{
    float temp=0, i; 
    send_temp(0xAA);
    dq.input();
    for(i=0 ; i<9 ; i++)
    {
        clk=0;
        wait_us(5);
        clk=1;
        wait_us(5);
        if(dq==1)
        {
            if(i<8)                
                temp=temp+pow(2.f,i);     
            else
                temp=temp*(-1);
            wait_us(5);
        }  
    }
    rst=0;
    return(temp/2);
}

void init_temp()    //Inicializa o sensor de temperatura DS1620
{
    send_temp(0x0C);
    send_temp(0xEE);
}

void send_email(char mensag[141])   //Envia email
{
    int fail=0;
    pc.printf("\n\n\r");
    EmailMessage msg;
    msg.setFrom(FROM_ADDRESS);
    msg.addTo(email);
    msg.printf("Subject: ALERTA!!!\n");
    msg.printf("%s\n",mensag);
    msg.printf(" \r\n");
    msg.printf("/Gaiola de Gerbos/\n");
    fail=smtp.send(&msg);
    pc.printf("\rSend result %d\n\r", fail);
    pc.printf("Last response | %s", smtp.getLastResponse().c_str());
    if(fail!=0)
        pc.printf("\n\n\remail nao enviado: ERRO de Conexao!");
    else
        pc.printf("\n\remail enviado: \"%s\"",mensag);
}

///////////////////////////////////////////////// Programa Principal: ///////////////////////////////////////////////////////

int main()
{   
    Timer t1,t2;
    char text[141];
    int erro=0,deltat=1;  // deltat - define a margem de erro da temperatura, a qual acrescentada ao limite desliga o aquecedor ou ventoinha
    clrscr();
    pc.printf("A estabelecer ligacao...\n\n\r");
        
    ///////////////////////////////////// CONNECT ETHERNET ///////////////////////////////////////////
    EthernetNetIf eth(HOSTNAME);
    EthernetErr ethErr;
    int count = 0;
    do {
        pc.printf("Tentativa %d...\n\r", ++count);
        ethErr = eth.setup();
        if (ethErr) pc.printf("Timeout\n\n\r", ethErr);
    } while (ethErr != ETH_OK && count!=3);
    
    if(ethErr != ETH_OK)
        pc.printf("Conexao Falhada! Sistema em Modo OFFLINE\n\n\r");
    else
        pc.printf("\nConexao OK! Sistema ONLINE\n\n\r");
    const char* hwAddr = eth.getHwAddr();
    pc.printf("HW address : %02x:%02x:%02x:%02x:%02x:%02x\n\r",hwAddr[0], hwAddr[1], hwAddr[2],hwAddr[3], hwAddr[4], hwAddr[5]);

    IpAddr ethIp = eth.getIp();
    pc.printf("IP address : %d.%d.%d.%d\n\n\r", ethIp[0], ethIp[1], ethIp[2], ethIp[3]);
    
    NTPClient ntp;
    printf("NTP setTime...\n");
    Host server(IpAddr(), 123, "pool.ntp.org");
    printf("\rResult : %d\n", ntp.setTime(server));

    time_t ctTime = time(NULL);
    printf("\n\rTime is now (UTC): %d %s\n\r", ctTime, ctime(&ctTime));
    
    Host host(IpAddr(), PORT, SERVER);
    SMTPClient smtp(host, DOMAIN, USER, PASSWORD, SMTP_AUTH_PLAIN);
    
    /////////////////////////////////////////CONNECT WEBSITE//////////////////////////////////////////////
    Base::add_rpc_class<DigitalOut>();
    
    FSHandler::mount("/local", "/files"); //Mount /webfs path on /files web path
    FSHandler::mount("/local", "/"); //Mount /webfs path on web root path
  
    svr.addHandler<SimpleHandler>("/hello");
    svr.addHandler<RPCHandler>("/rpc");
    svr.addHandler<FSHandler>("/files");
    svr.addHandler<FSHandler>("/"); //Default handler
    
    svr.bind(80);
  
    /////////////////////////////////////////// Sistema Geral: ////////////////////////////////////////////////////////////////
    pc.printf("\n\rSistema Iniciado\n\r");
    erro=ler();
    if(erro==1)
        config(1);  //se nao houver ficheiro de configuracao, entra no modo de configuracao para definir os parametros
    init_temp();
    t1.start();
    t2.start();
    temp=read_temp();
    wait(1);
    while(1)
    {   
        //////////////////////////////////////////Configuracao:///////////////////////////////////////
                
        if(conf==1)         //se o botao de configuracao for pressionado, entra no modo de configuracao
            config(0);
        
        ////////////////////////////////////Leitura de Temperatura://////////////////////////////////
        
        if(t1.read()>1)     //de 1 em 1 segundo faz a leitura da temperatura
        {
            temp=read_temp();
            pc.printf("\n\rTemperatura: %.1f C",temp);    
            t1.reset();
        }    
        
        ////////////////////////////////////Actualizacao do Grafico://////////////////////////////////
        
        if(t2.read()>60)    //de minuto a minuto, actualiza o API do grafico de temperatura
        {
            sprintf(urlBuffer, "%s?key=%s&field1=%.1f", thingSpeakUrl, thingSpeakKey, temp);
            printf("\n\n\rRequest to %s\r\n", urlBuffer);
            HTTPText resp;
            HTTPResult res = http.get(urlBuffer, &resp);
            if (res == HTTP_OK)
                printf("Result :\"%s\"\r\n", resp.gets());
            else
                printf("Error %d\r\n", res);
            t2.reset();                
        }
               
        /////////////////////////////////////////////Comida://////////////////////////////////////////
         
        if(foodsense==0)    //Food Sensor -> Tem comida = 1 ; Nao tem comida = 0;
        {
            led1=1;
            if(food==0)     //se e' a primeira indicacao de que nao ha comida, envia email e sms
            {
                send_email(msg3); 
                sms(msg3);
                food=1;
            }
            motor=1;
        }
        else
        {
            led1=0;
            motor=0;
            if(food==1)     //se e' a primeira indicacao de que ha comida, envia email e sms 
            {
                send_email("Comida Reposta.");
                sms("Comida Reposta.");
                food=0;
            }
        }
        
        /////////////////////////////////////////////Agua:////////////////////////////////////////////
        
        if(lowwater==0)     //LowWater -> Bebedouro vazio = 0 ; Bebedouro nao vazio = 1;
        { 
            led2=1;
            if(water==0)    //se e' a primeira indicacao de que acabou a agua, envia sms e email
            {
                send_email(msg2); 
                sms(msg2);
                water=1;
            }
            valve=1;
        }
                
        if(highwater==1)    //HighWater -> Bebedouro cheio = 1 ; Bebedouro nao cheio = 0;
        { 
            led2=0;
            valve=0;
            if(water==1 && lowwater==1) //se e' a primeira indicacao de que a agua foi totalmente reposta, envia email e sms
            {
                send_email("Agua Reposta."); 
                sms("Agua Reposta.");
                water=0;
            }
        }
        
        //////////////////////////////////////////Temperatura:///////////////////////////////////////
               
        if(temp>tmax)       //se a temperatura for superior a temperatura maxima
        {
            led4=1;
            fan=1;
            heater=0;
            if(temperature==0)      //se e' a primeira indicacao de que a temperatura ultrapassou o limite:
            {
                sprintf(text,"%s: %.1f C",msg1,temp);
                send_email(text); 
                sms(text);
                temperature=1;
            }         
        }
        else
        {
            if(temp<tmin)   //se a temperatura for inferior a temperatura minima
            {
                led3=1;
                heater=1;
                fan=0;
                if(temperature==0)  //se e' a primeira indicacao de que a temperatura ultrapassou o limite:
                {
                    sprintf(text,"%s: %.1f C",msg1,temp);
                    send_email(text); 
                    sms(text);
                    temperature=1;
                }
            }
            else            //se a temperatura esta dentro dos limites
            {
                led3=0;
                led4=0;
                if(temp<=tmax-deltat)   //se a temperatura for inferior 'a maxima menos deltaT, desliga ventoinha
                    fan=0;
                if(temp>=tmin+deltat)   //se a temperatura for superior 'a minima mais deltaT, desliga aquecedor
                    heater=0;
                if(temperature==1 && fan==0 && heater==0)   //se for a primeira indicacao de que a temperatura entrou chegou ao normal
                {
                    sprintf(text,"Temperatura Restabelecida: %.1f C",temp);
                    send_email(text); 
                    sms(text);
                    temperature=0;
                }
            }
        }
    Net::poll();    //mantem a pagina internet online
    }
} 






