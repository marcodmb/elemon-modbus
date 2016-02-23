// Revalco 1RANM8 RS485/modbus-rtu reader
// 2012-2016 - Marco D'Ambrosio - info@marcodambrosio.it

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "modbus.h"
#include "modbus.c"
#include "modbus-rtu.h"
#include "modbus-rtu.c"
#include "modbus-data.c"
#include <time.h>
#include <mysql.h>

#define read_pause 150000
#define serialport "/dev/ttyUSB0"
#define baudrate 9600
#define dbip "10.20.30.40"
#define dbusername "dbuser"
#define dbname "dbname"
#define dbpassword "password"

MYSQL *conn;

uint16_t lowreg(uint16_t reg)
{
	uint16_t low;
	low=reg&0xFF;
	return low;
}

uint16_t highreg(uint16_t reg)
{
	uint16_t high;
	high=reg&0xFF00;
	return high>>8;
}

uint16_t swapreg(uint16_t reg)
{
	uint16_t low;
	uint16_t high;
	low=reg&0xFF00;
	low=low>>8;
	high=reg&0xFF;
	high=high<<8;
	return low+high;
}

uint32_t combine2reg (uint16_t reg1,uint16_t reg2)
{
	uint32_t val;
	val=reg1;
	val=val<<16;
	return val+reg2;
}

void database_connect (void) {
	conn=mysql_init(NULL);
        if (mysql_real_connect(conn, dbip, dbusername, dbpassword, dbname, 0, NULL, 0) == NULL) {
                printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
        } else {
                printf("MySQL connection is OK!\n");
        }
}

void database_restart (void) {
	mysql_close(conn);
	database_connect();
}

int main(int argc, char *argv[])
{
	modbus_t *ctx;
	uint16_t data[16];
	uint16_t Vscale;
	uint16_t Iscale;
	uint16_t VTF;
	uint16_t CTF;
	uint16_t PQS;
	uint16_t PSQTF;
	float frequency;
	float voltage;
	float voltage_peak;
	float current;
	float current_peak;
	float activepower;
	char activepowersign;
	float apparentpower;
	float reactivepower;
	char reactivepowersign;
	float absolute_max_active_energy;
	float absolute_max_reactive_energy;
	float absolute_last_active_energy;
	float absolute_last_reactive_energy;
	float total_positive_active_energy;
	float total_negative_active_energy;
	float total_positive_reactive_energy;
	float total_negative_reactive_energy;
	uint16_t powerfactor_lreg;
	uint16_t powerfactor_hreg;
	//char powerfactor_sign;
	char powerfactor_ic;
	float powerfactor_real;
	float vthd;
	float cthd;
	float Vcrest;
	float Icrest;
	int esito_lettura;
	int errori_lettura;
//	uint16_t activepower_lreg;
//	uint16_t activepower_hreg;
	
	int debug = 1;

	typedef struct valori_db {
                char name[20];
                char value[10];
        } dbfill;
        dbfill dbvar[30];
	int dbnumval;
	int cont;	

	char *query[500];
	char *query_fields[100];
	char *query_values[100];
 	
	database_connect();

	ctx = modbus_new_rtu(serialport,baudrate,'N',8,1);
	if (ctx == NULL) {
    		fprintf(stderr, "RS485/modbus-rtu connection error\n");
    		return -1;
	}
	
	modbus_connect(ctx);
	modbus_set_debug(ctx,0);
	modbus_set_slave(ctx,1);
	
	printf("************************************************\n");
	printf("Revalco 1RANM8 RS485/modbus-rtu reader\n");
	printf("2012 - Marco D'Ambrosio - info@marcodambrosio.it\n");
	printf("************************************************\n");
	printf("Starting monitoring daemon...\n\n");

	if (debug==1) {
		printf("*********************************************\n");
		printf("RS485 %dbps connection - Parameters:\n",baudrate);
		printf("*********************************************\n");
	}	

	// Lettura registri 0x10
        usleep(read_pause);
	esito_lettura = modbus_read_registers(ctx,16,1,data);
	if (esito_lettura != -1)
	{
        	VTF=swapreg(data[0]);
		if (debug==1) {
			printf("VTF = %u\n",VTF);
		}
	}
	
	// Lettura registri 0x11
        usleep(read_pause);
        esito_lettura = modbus_read_registers(ctx,17,1,data);
	if (esito_lettura != -1)
	{
        	CTF=swapreg(data[0]);
		if (debug==1) {
			printf("CTF = %u\n",CTF);
		}
	}
	
	// Lettura registri 0x80-0x82
        usleep(read_pause);
        esito_lettura = modbus_read_input_registers(ctx,128,3,data);
	if (esito_lettura != -1)
	{
        	Vscale=swapreg(data[0]);
        	Iscale=swapreg(data[1]);
        	PQS=swapreg(data[2]);
		if (debug==1) {
			printf("Vscale = %u\n",Vscale);
			printf("Iscale = %u\n",Iscale);
			printf("PQS = %u\n",PQS);
		}	
	}

	// Lettura registri 0x8A
        usleep(read_pause);
        esito_lettura = modbus_read_input_registers(ctx,138,1,data);
	if (esito_lettura != -1)
	{
        	PSQTF=swapreg(data[0]);
		if (debug==1) {		
			printf("PSQTF = %u\n",PSQTF);
		}
	}
	
	if (debug==1) {		
		printf("*********************************************\n\n");
	}	

	// Lettura registri 0x44B-456
        usleep(read_pause);
        esito_lettura = modbus_read_input_registers(ctx,1099,12,data);
	if (esito_lettura != -1)
	{
        	absolute_max_active_energy = (float)combine2reg(swapreg(data[1]),swapreg(data[0]))/100;
        	absolute_max_reactive_energy = (float)combine2reg(swapreg(data[4]),swapreg(data[3])/100);
        	absolute_last_active_energy = (float)combine2reg(swapreg(data[7]),swapreg(data[6]))/100;
        	absolute_last_reactive_energy = (float)combine2reg(swapreg(data[10]),swapreg(data[9])/100);
		if (debug==1) {
			printf("*********************************************\n");
			printf("Livelli massimi di energia consumata:\n");
			printf("*********************************************\n");
			printf("Energia attiva (abs)\t\t%4.2f\tkWh\n",absolute_max_active_energy);
			printf("Energia reattiva (abs)\t\t%4.2f\tkVArh\n",absolute_max_reactive_energy);
			printf("Energia attiva (last)\t\t%4.2f\tkWh\n",absolute_last_active_energy);
			printf("Energia reattiva (last)\t\t%4.2f\tkVArh\n",absolute_last_reactive_energy);
			printf("*********************************************\n\n");
		}
	}
	
	// Lettura registri 0x409-0x414
        usleep(read_pause);
        esito_lettura = modbus_read_input_registers(ctx,1033,12,data);
	if (esito_lettura != -1)
	{
        	total_positive_active_energy = (float)(combine2reg(swapreg(data[1]),swapreg(data[0]))+swapreg(data[2])*100000000)/100;
        	total_negative_active_energy = (float)(combine2reg(swapreg(data[4]),swapreg(data[3]))+swapreg(data[5])*100000000)/100;
        	total_positive_reactive_energy = (float)(combine2reg(swapreg(data[7]),swapreg(data[6]))+swapreg(data[8])*100000000)/100;
        	total_negative_reactive_energy = (float)(combine2reg(swapreg(data[10]),swapreg(data[9]))+swapreg(data[11])*100000000)/100;
		if (debug==1) {
			printf("*********************************************\n");
			printf("Energia totale consumata:\n");
			printf("*********************************************\n");
			printf("Energia attiva (+)\t\t%6.2f\tkWh\n",total_positive_active_energy);
			printf("Energia attiva (-)\t\t%6.2f\tkWh\n",total_negative_active_energy);
			printf("Energia reattiva (+)\t\t%6.2f\tkVArh\n",total_positive_reactive_energy);
			printf("Energia reattiva (-)\t\t%6.2f\tkVArh\n",total_negative_reactive_energy);
			printf("*********************************************\n\n");
		}
	}
	
	sleep(2);

	while (1) {
		errori_lettura = 0;
		dbnumval = 0;
		time_t mytime = time(0);
		
		if (debug==1) {
			printf("*********************************************\n");
			printf("* %s*********************************************\n",asctime(localtime(&mytime)));
		}

		// Lettura registri 0x100-0x102
        	usleep(read_pause);
		esito_lettura = modbus_read_input_registers(ctx,256,3,data);
		if (esito_lettura != -1)
		{
			frequency = (float)swapreg(data[0])/100;
			voltage = (float)swapreg(data[1])*Vscale/1024*VTF/10;
			voltage_peak = (float)swapreg(data[2])*Vscale/1024*VTF/10;
			if (debug==1) {
				printf("Frequenza rete\t\t%2.2f\t\tHz\n",frequency);
				printf("Tensione\t\t%3.2f\t\tV\n",voltage);
				printf("Tensione di picco\t%3.2f\t\tV\n",voltage_peak);
			}
                                        sprintf(dbvar[dbnumval].name,"frequency");
                                        sprintf(dbvar[dbnumval].value,"%2.2f",frequency);
                                        dbnumval++;
                                        sprintf(dbvar[dbnumval].name,"voltage");
                                        sprintf(dbvar[dbnumval].value,"%3.2f",voltage);
                                        dbnumval++;
                                        sprintf(dbvar[dbnumval].name,"voltage_peak");
                                        sprintf(dbvar[dbnumval].value,"%3.2f",voltage_peak);
                                        dbnumval++;
			if (voltage>0)
			{
				Vcrest=(float)swapreg(data[2])/swapreg(data[1]);
				if (debug==1) {
					printf("Fattore di cresta (V)\t%1.2f\n",Vcrest);
				}
					sprintf(dbvar[dbnumval].name,"Vcrest");
					sprintf(dbvar[dbnumval].value,"%1.2f",Vcrest);
					dbnumval++;
			}
			else
			{
				Vcrest=0;
				if (debug==1) {
					printf("Fattore di cresta (V)\t-.--\n");
				}
			}
		} else {
			errori_lettura++;
		}

		// Lettura registri 0x105-0x106
        	usleep(read_pause);
		esito_lettura = modbus_read_input_registers(ctx,261,2,data);
		if (esito_lettura != -1)
		{
			current = (float)swapreg(data[0])*Iscale/1024*CTF/1000;
			current_peak = (float)swapreg(data[1])*Iscale/1024*CTF/1000;
			if (debug==1) {
				printf("Corrente\t\t%3.2f\t\tA\n",current);
				printf("Corrente di picco\t%3.2f\t\tA\n",current_peak);
			}
                                        sprintf(dbvar[dbnumval].name,"current");
                                        sprintf(dbvar[dbnumval].value,"%3.2f",current);
					dbnumval++;
                                        sprintf(dbvar[dbnumval].name,"current_peak");
                                        sprintf(dbvar[dbnumval].value,"%3.2f",current_peak);
                                        dbnumval++;
			if (current>0) {
				Icrest=(float)swapreg(data[1])/swapreg(data[0]);
				if (debug==1) {
					printf("Fattore di cresta (I)\t%1.2f\n",Icrest);
				}
					sprintf(dbvar[dbnumval].name,"Icrest");
                                        sprintf(dbvar[dbnumval].value,"%1.2f",Icrest);
                                        dbnumval++;
			}
			else
			{
				Icrest=0;
				if (debug==1) {
					printf("Fattore di cresta (I)\t-.--\n");
				}
			}
		} else {
			errori_lettura++;
		}
		
		// Lettura registri 0x109-0x10F
        	usleep(read_pause);
		esito_lettura = modbus_read_input_registers(ctx,265,7,data);
		if (esito_lettura != -1)
		{
			activepower = (float)combine2reg(swapreg(data[1]),swapreg(data[0]))*Vscale*Iscale/10485760*PSQTF/1000;
			reactivepower = (float)combine2reg(swapreg(data[5]),swapreg(data[4]))*Vscale*Iscale/10485760*PSQTF/1000;
			apparentpower = (float)combine2reg(swapreg(data[3]),swapreg(data[2]))*Vscale*Iscale/10485760*PSQTF/1000;
			powerfactor_lreg = lowreg(swapreg(data[6]));
			powerfactor_hreg = highreg(swapreg(data[6]));
			if ((powerfactor_hreg&0x10)==0x10) {
				activepowersign='-';
			} else {
				activepowersign='+';
			}
			if ((powerfactor_hreg&0x2)==0x2) {
                                reactivepowersign='+';
                        } else {
                                reactivepowersign='-';
                        }
			if (debug==1) {
				printf("Potenza attiva\t\t%c%4.2f\t\tW\n",activepowersign,activepower);
				printf("Potenza reattiva\t%c%4.2f\t\tVAr\n",reactivepowersign,reactivepower);
				printf("Potenza apparente\t+%4.2f\t\tVA\n",apparentpower);
			}
                                        sprintf(dbvar[dbnumval].name,"activepower");
                                        sprintf(dbvar[dbnumval].value,"%c%4.2f",activepowersign,activepower);
                                        dbnumval++;
                                        sprintf(dbvar[dbnumval].name,"reactivepower");
                                        sprintf(dbvar[dbnumval].value,"%c%4.2f",reactivepowersign,reactivepower);
                                        dbnumval++;
                                        sprintf(dbvar[dbnumval].name,"apparentpower");
                                        sprintf(dbvar[dbnumval].value,"%4.2f",apparentpower);
                                        dbnumval++;
			if (powerfactor_lreg == 255 || ((powerfactor_hreg&0x20)==0x20))  {
				if (debug==1) {
					printf("Fattore di potenza\t-.--\t\tPf\n");
				}
			} else if ((powerfactor_hreg&0x80)==0x80){
				if (debug==1) {
					printf("Fattore di potenza\t1\t\tPf\n");
				}
					sprintf(dbvar[dbnumval].name,"powerfactor_real");
                                        sprintf(dbvar[dbnumval].value,"1.00");
                                        dbnumval++;
			} else {
				powerfactor_real=(float)powerfactor_lreg/100;
					//if ((powerfactor_hreg&0x10)==0x10) {
					//	powerfactor_sign='-';
					//} else powerfactor_sign='+';
					if ((powerfactor_hreg&0x1)==0x1) {
                                        	powerfactor_ic='c';
                                	} else { powerfactor_ic='i'; }
	                                sprintf(dbvar[dbnumval].name,"powerfactor_ic");
                                        sprintf(dbvar[dbnumval].value,"'%c'",powerfactor_ic);
                                        dbnumval++;
                                        sprintf(dbvar[dbnumval].name,"powerfactor_real");
                                        //sprintf(dbvar[dbnumval].value,"%c%1.2f",powerfactor_sign,powerfactor_real);
                                        sprintf(dbvar[dbnumval].value,"%1.2f",powerfactor_real);
                                        dbnumval++;
				if (debug==1) {
					//printf("Fattore di potenza\t%c%1.2f%c\t\tPf\n",powerfactor_sign,powerfactor_real,powerfactor_ic);
					printf("Fattore di potenza\t%1.2f%c\t\tPf\n",powerfactor_real,powerfactor_ic);
				}
			}
		} else {
                        errori_lettura++;
                }

		
		// Lettura registri 0x500-0x501
        	usleep(read_pause);
		esito_lettura = modbus_read_input_registers(ctx,1280,2,data);
		if (esito_lettura != -1)
		{
			vthd = (float)swapreg(data[0])/10;
			cthd = (float)swapreg(data[1])/10;
			if (debug==1) {
				printf("Distorsione tensione\t%2.1f\t\t%%\n",vthd);
			}
				sprintf(dbvar[dbnumval].name,"vthd");
                                sprintf(dbvar[dbnumval].value,"%2.1f",vthd);
                                dbnumval++;
			if (current>0)
			{
				if (debug==1) {
					printf("Distorsione corrente\t%2.1f\t\t%%\n",cthd);
				}
					sprintf(dbvar[dbnumval].name,"cthd");
                                	sprintf(dbvar[dbnumval].value,"%2.1f",cthd);
                                	dbnumval++;
			} else {
				if (debug==1) {
					printf("Distorsione corrente\t--.-\t\t%%\n");
				}
			}
		} else {
                        errori_lettura++;
                }

		
		if (debug==1) {
			printf("*********************************************\n\n");
		}
		
		if (errori_lettura==0 && dbnumval >= 1) {
			query_fields[0]='\0';
			query_values[0]='\0';
			query[0]='\0';
			for (cont=0;cont<dbnumval;cont++){
				if (cont!=0){
					strcat(query_fields,",");
					strcat(query_values,",");
				}
				strcat(query_fields,dbvar[cont].name);
				strcat(query_values,dbvar[cont].value);
			}
			strcat(query,"INSERT INTO data (");
			strcat(query,query_fields);
			strcat(query,") VALUES (");
			strcat(query,query_values);
			strcat(query,");");
			if (mysql_query(conn, query))
			{
			     printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
			     database_restart();
			}
		} else {
			printf("DB insert not committed, %d read errors and %d values collected!\n",errori_lettura,dbnumval);
		}
	
		sleep(2);
	}

	mysql_close(conn);
	modbus_close(ctx);
	modbus_free(ctx);
}
