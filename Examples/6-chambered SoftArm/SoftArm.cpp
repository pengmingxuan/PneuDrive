/*
 * SoftArm.c
 *
 *  Created on: May 12, 2018
 *      Author: 402072495
 */
#include "SoftArm.h"
#include "stdio.h"


const static float Patm=101325;
const static float PaPerPSI=6895;

const static float preOpeningMinN=-0.7;
const static float preOpeningMaxN=-0.3;

const static float preOpeningMinP=0.11;
const static float preOpeningMaxP=0.3;

static float preOpeningLimArray[6][4]={
		{preOpeningMinN,preOpeningMaxN,preOpeningMinP,preOpeningMaxP},//DIP
		{preOpeningMinN,preOpeningMaxN,preOpeningMinP,preOpeningMaxP},//PIP
		{preOpeningMinN,preOpeningMaxN,preOpeningMinP,preOpeningMaxP},//MIP
		{preOpeningMinN,preOpeningMaxN,preOpeningMinP,preOpeningMaxP},//AB
		{preOpeningMinN,preOpeningMaxN,preOpeningMinP,preOpeningMaxP},//AD
		{preOpeningMinN,preOpeningMaxN,preOpeningMinP,preOpeningMaxP},//DIP
};



/*************************SOFT ARM**************************
 *
 ***********************************************************/
SOFT_ARM::SOFT_ARM(int num)
{
	static float initialBellowConfigurationAngle[BELLOWNUM]={0, M_PI/3, 2*M_PI/3, M_PI, -2*M_PI/3 ,-M_PI/3}; //(-PI~PI)
	basePlatform = new PNEUDRIVE(num);
	for(int i=0;i<BELLOWNUM;i++)
	{
		bellows[i]=basePlatform->chambers[i];

	}
	length = 0.3;
	angle = 0;
	bending =0;

	pressureBase = 0;
	openingBase = 0;
	frequency = 40;
	velocity = 1;
	lengthAnalogPort = 9;

	alpha = 0;
	beta = 0;
	startControl=0;
	controlTime=0;
	valveOpen=0;
	wp=0;
	startDemo=0;
	interestedBellow = 0;
	Rmin = 8000;
	Rmax = 30000;
	rawAngle=0,
	rawAmplitude=0,
	rawAmplitudeMax=0,
	buttonCheckTime[0]=100;
	buttonCheckTime[1]=100;
	buttonCheckTime[2]=100;
	angleCommand=0;
	amplitudeCommand=0;
	commandSource=pressureControl;
	frequencyDirty=0;

 	k0=500;
 	length0=0.412;
 	crossA=2.3e-3;
 	radR=0.3;
 	radr=0.06;

	psource_lowerlimit=50000;
	psource_upperlimit=psink_upperlimit+30000;
	psink_upperlimit=-20000;
	psink_lowerlimit=psink_upperlimit-10000;
	memcpy(OpeningLimArray,preOpeningLimArray,sizeof(preOpeningLimArray));

	/*Init Bellows configuration*/
	for(int i=0;i<BELLOWNUM;i++){

		bellowConfigurationRadius = 0.5;
		bellowConfigurationAngle[i] =  initialBellowConfigurationAngle[i];

		bellowConfigurationPositionX[i] = bellowConfigurationRadius * arm_cos_f32(bellowConfigurationAngle[i]);
		bellowConfigurationPositionY[i] = bellowConfigurationRadius * arm_sin_f32(bellowConfigurationAngle[i]);

		basePlatform->chambers[i]->openingMinN=OpeningLimArray[i][0];
		basePlatform->chambers[i]->openingMaxN=OpeningLimArray[i][1];
		basePlatform->chambers[i]->openingMinP=OpeningLimArray[i][2];
		basePlatform->chambers[i]->openingMaxP=OpeningLimArray[i][3];

		pressureCommandMin[i]=-100000;
		pressureCommandMax[i]=200000;
		pressureCommand[i]=0;

	}
}


void SOFT_ARM::setup()
{
	for(int i=0;i<BELLOWNUM;i++)
	{
		//bellows[i]->attach(2*i,2*i+1,i);\\onboard resources
		bellows[i]->attach(i*2+BUILTIN_PWM_NUM+PWMBOARDSPI_CHANNELNUM,i*2+1+BUILTIN_PWM_NUM+PWMBOARDSPI_CHANNELNUM,31-i+BUILTIN_ANA_IN_NUM);
//		bellows[i]->setSensorRange_GaugePa(0.5,4.5,-Patm,60*PaPerPSI); //(-Patm ~ 60PSI)

	}
	lengthAnalogPort = BUILTIN_ANA_IN_NUM;
	basePlatform->pSource.attach(0,31-10+BUILTIN_ANA_IN_NUM);
	basePlatform->pSink.attach(1,31-11+BUILTIN_ANA_IN_NUM);


//	basePlatform->pSource.pressureSensor.setSensorRange_GaugePa(0.5,4.5,-Patm,60*PaPerPSI);
//	basePlatform->pSink.pressureSensor.setSensorRange_GaugePa(0.5,4.5,-Patm,60*PaPerPSI);


	basePlatform->setupPlatform();
	for(int i=0;i<BUILTIN_PWM_NUM;i++)
			PWMWriteDuty(i,0.4);
}

void SOFT_ARM::loop()
{
	readLength();
	readPressureAll();
	if(frequencyDirty == 1)
		{
			basePlatform->writeFrequency(frequency);
			frequencyDirty=0;
		}
	if(valveOpen == 1)
	{
		writeOpeningAll(-0.95);
	}
	else if(valveOpen == 2)
	{
		writeOpeningAll(0);
		valveOpen++;
	}
	if(startControl)
	{
	//	basePlatform->pSource.maintainPressure(psource_lowerlimit,psource_upperlimit);
		basePlatform->pSink.maintainPressure(psink_lowerlimit,psink_upperlimit);

		if(commandSource==joyStickControl)
		{
			joyStickController(0);
		}
		else if(commandSource==mannualControl)
		{
			for(int i=0;i<BELLOWNUM;i++)
				basePlatform->chambers[i]->writeOpening(basePlatform->chambers[i]->opening);
		}
		else if(commandSource==pressureControl)
		{
			controlPressureAll();
		}
		else if(commandSource==positionControl)
		{

		}

//		switchValveStatus();
	}
	//storeLast500ms(controlCLK);
}

void SOFT_ARM::display()
{
	static int16_t pCommandToSend[BELLOWNUM];
	static int16_t pToSend[BELLOWNUM];
	for(int i=0;i<BELLOWNUM;i++)
	{
		pCommandToSend[i]=round(bellows[i]->pressureFil/1000);
		pToSend[i]=round(pressure[i]/1000);
	}
	int free=(int)frequency;
	int lengthh=(int)(length*1000);
	int psourcee=(int)(basePlatform->pSource.readPressure()/1000);
	int psinkk=(int)(basePlatform->pSink.readPressure()/1000);
/*	static float outputem[16]={0};
	for(int i=0;i<16;i++)
		outputem[i]=AnalogRead(i);*/

/*	printf("%4.2f %1.3f %1.3f %1.3f %1.3f %1.3f %1.3f %1.3f %1.3f %1.3f %1.3f %1.3f %1.3f %1.3f %1.3f %1.3f %1.3f\r\n",
		   HAL_GetTick()/1000.0f,
		   outputem[0],outputem[1],outputem[2],outputem[3],outputem[4],outputem[5],outputem[6],outputem[7],
		   outputem[8],outputem[9],outputem[10],outputem[11],outputem[12],outputem[13],outputem[14],outputem[15]);*/
	 printf("%4.2f %4d	%4d	%4d	%4d	%4d	%4d	%4d	%4d	%4d	%4d	%4d	%4d	%d	%5d	%4d	%4d	%3.1f	%3.1f	%3.1f	%3.4f	%3.4f	%3.4f	%3.4f\r\n",
			  HAL_GetTick()/1000.0f,
			pCommandToSend[0],
			pToSend[0],
			pCommandToSend[1],
			pToSend[1],
			pCommandToSend[2],
			pToSend[2],
			pCommandToSend[3],
			pToSend[3],
			pCommandToSend[4],
			pToSend[4],
			pCommandToSend[5],
			pToSend[5],
			free,
			lengthh,
			psourcee,
			psinkk,
			imuData.angleX,
			imuData.angleY,
			imuData.angleZ,
			imuData.q0,
			imuData.q1,
			imuData.q2,
			imuData.q3);
}

void SOFT_ARM::receiveCommand(char *pSerialReceiveBuffer)
{

		char str[200];
		uint32_t b;
		float c;

		if(pSerialReceiveBuffer[0] == 'g')
		{
			startControl=1;
			startDemo = 1;
		}
		else if(pSerialReceiveBuffer[0] == 'f')
		{
			sscanf(&pSerialReceiveBuffer[2], "%f",&(frequency));
			frequencyDirty = 1;
		}
		else if(pSerialReceiveBuffer[0] == 's')
		{
			startControl=0;
			startDemo = 0;
			valveOpen=1;
			basePlatform->pSink.stop();
			basePlatform->pSource.stop();
			for(int i=0;i<BELLOWNUM;i++)
				pressureCommand[i] = 0;
			commandSource=pressureControl;
		}

	/*	else if(pSerialReceiveBuffer[0]=='h')
		{
			sscanf(pSerialReceiveBuffer, "%s %lu %f", &str[0],  &b,  &c);
			DigitalWrite(b,1);
		}*/

		else if(pSerialReceiveBuffer[0] == 'P')
		{
			float pressureTem[BELLOWNUM];
			float *p=pressureTem;
			sscanf(pSerialReceiveBuffer, "%s %f %f %f %f %f %f",str,p, p+1, p+2, p+3, p+4, p+5);
			for(int i=0;i<BELLOWNUM;i++)
			{
				pressureCommand[i] = pressureTem[i]*1000.0f;
			}
			commandSource=pressureControl;
			wp=1;
		}
		else if(pSerialReceiveBuffer[0] == 'p')
		{
			if (pSerialReceiveBuffer[1]==' '){
				float pressureTem[BELLOWNUM];
				float *p=pressureTem;
				int argNum=sscanf(&pSerialReceiveBuffer[2], "%f %f %f %f %f %f",p, p+1, p+2, p+3, p+4, p+5);
				if(argNum==1){
					for(int i=0;i<BELLOWNUM;i++)
					{
						pressureCommand[i] = pressureTem[0]*1000.0f;
					}
				}
				else if(argNum==2){
					interestedBellow = (int)(round(pressureTem[1]));
					pressureCommand[interestedBellow]=pressureTem[0]*1000;
				}
				else if(argNum==6){
					for(int i=0;i<BELLOWNUM;i++)
					{
						pressureCommand[i] = pressureTem[i]*1000.0f;
					}
				}
				commandSource=pressureControl;
				wp=1;
			}
			else if (pSerialReceiveBuffer[1]=='o'){
				float pressureDes=0;
				sscanf(&pSerialReceiveBuffer[3], "%f",&pressureDes);
				psink_upperlimit=pressureDes*1000;
				psink_lowerlimit=psink_upperlimit-10000;
			}
			else if (pSerialReceiveBuffer[1]=='i'){
				float pressureDes=0;
				sscanf(&pSerialReceiveBuffer[3], "%f",&pressureDes);
				psource_upperlimit=pressureDes*1000;
				psource_lowerlimit=psource_upperlimit-10000;
			}
		}
		else if(pSerialReceiveBuffer[0] == 'z')
		{
			for(int i=0;i<BELLOWNUM;i++)
				basePlatform->chambers[i]->pressureSensor.vMin = AnalogRead(basePlatform->chambers[i]->pressureSensor.AnalogPort);
		}
		else if(pSerialReceiveBuffer[0] == 'o')
		{
			valveOpen++;
		}
		else if(pSerialReceiveBuffer[0] == 'O')
		{
			commandSource=mannualControl;
			int num=0;
			float op=0;
			sscanf(&pSerialReceiveBuffer[2], "%d %f",&num,&op);
			basePlatform->chambers[num]->opening=op;
		}
		else if(pSerialReceiveBuffer[0] == 'u')
		{
			if(pSerialReceiveBuffer[1] == '1')
			{
				float deflateMinN;
				int num;
				sscanf(&pSerialReceiveBuffer[3], "%f %d",&deflateMinN,&num);
				if(num<0)
				 for(int i=0;i<BELLOWNUM;i++){
					basePlatform->chambers[i]->openingMinN = deflateMinN;
					}
				else{
					basePlatform->chambers[num]->openingMinN = deflateMinN;
				}
			}
			else if(pSerialReceiveBuffer[1] == '2')
			{
				float deflateMaxN;
				int num;
				sscanf(&pSerialReceiveBuffer[3], "%f %d",&deflateMaxN,&num);
				if(num<0)
				 for(int i=0;i<BELLOWNUM;i++){
					basePlatform->chambers[i]->openingMaxN = deflateMaxN;
					}
				else{
					basePlatform->chambers[num]->openingMaxN = deflateMaxN;
				}
			}
		}
		else if(pSerialReceiveBuffer[0] == 'U')
			{
			if(pSerialReceiveBuffer[1] == '1')
					{
						float inflateMinP;
						int num;
						sscanf(&pSerialReceiveBuffer[3], "%f %d",&inflateMinP,&num);
						if(num<0)
						 for(int i=0;i<BELLOWNUM;i++){
							basePlatform->chambers[i]->openingMinP = inflateMinP;
							}
						else{
							basePlatform->chambers[num]->openingMinP = inflateMinP;
						}

					}
					else if(pSerialReceiveBuffer[1] == '2')
					{
						float inflateMaxP;
						int num;
						sscanf(&pSerialReceiveBuffer[3], "%f %d",&inflateMaxP,&num);
						if(num<0)
							 for(int i=0;i<BELLOWNUM;i++){
							basePlatform->chambers[i]->openingMaxP = inflateMaxP;
							}
						else{
							basePlatform->chambers[num]->openingMaxP = inflateMaxP;
						}
					}
			}
		else if(pSerialReceiveBuffer[0] == 'k')
			{
				if(pSerialReceiveBuffer[1] == 'p'){
				float kp[BELLOWNUM];
				float *p=kp;
				sscanf(pSerialReceiveBuffer, "%s %f",str,p);

				 for(int i=0;i<BELLOWNUM;i++){
					setKp(basePlatform->chambers[i]->pressureController->pPID, kp[0]);
				    }
				}

				else if(pSerialReceiveBuffer[1] == 'i'){
				float ki[BELLOWNUM];
				float *p=ki;
				sscanf(pSerialReceiveBuffer, "%s %f",str,p);

				 for(int i=0;i<BELLOWNUM;i++){
					setKi(basePlatform->chambers[i]->pressureController->pPID, ki[0]);
				    }
				}

				else if(pSerialReceiveBuffer[1] == 'd'){
				float kd[BELLOWNUM];
				float *p=kd;
				sscanf(pSerialReceiveBuffer, "%s %f",str,p);

				 for(int i=0;i<BELLOWNUM;i++){
					setKd(basePlatform->chambers[i]->pressureController->pPID, kd[0]);
				    }
				}

			}
		else if(pSerialReceiveBuffer[0] == 'd')
		{
			float deadzone;
				sscanf(pSerialReceiveBuffer, "%s %f",str,&deadzone);
			for(int i=0;i<BELLOWNUM;i++){
				basePlatform->chambers[i]->pressureDeadZone = deadzone*1000;
			}

		}
		else if(pSerialReceiveBuffer[0] == 'D')
		{
			uint32_t dd=0;
			float ff=0;
			sscanf(pSerialReceiveBuffer, "%s %f",str,&dd,&ff);
			PWMWriteDuty(dd,ff);

		}
		else if(pSerialReceiveBuffer[0] == 'Q')
		{
			float kalmanq;
				sscanf(pSerialReceiveBuffer, "%s %f",str,&kalmanq);
			for(int i=0;i<BELLOWNUM;i++){
				kalman_setQ(basePlatform->chambers[i]->pressureController->pKalmanFilter,&kalmanq);
			}

		}
		else if(pSerialReceiveBuffer[0] == 'R')
		{
			float kalmanR;
				sscanf(pSerialReceiveBuffer, "%s %f",str,&kalmanR);
			for(int i=0;i<BELLOWNUM;i++){
				kalman_setR(basePlatform->chambers[i]->pressureController->pKalmanFilter,&kalmanR);
			}

		}
		else if(pSerialReceiveBuffer[0] == 'c')
		{
			sscanf(&pSerialReceiveBuffer[2], "%d",&(interestedBellow));
		}
		else if(pSerialReceiveBuffer[0] == 'A')
		{
			if(pSerialReceiveBuffer[3] == 'A')
				{
				wp=0;
			}
		}
		else if(pSerialReceiveBuffer[0] == 'J')
			{

				int btn;
				int32_t xx=0;
				int32_t yy=0;
				int32_t zz=0;
				int32_t rxx=0;
				int32_t ryy=0;
				int32_t rzz=0;

				sscanf(&pSerialReceiveBuffer[2], "%d %ld %ld %ld %ld %ld %ld",&btn,&xx,&yy,&zz,&rxx,&ryy,&rzz);
				joyStick.receiveJoyStickCommand((uint8_t)btn,xx,yy,zz,rxx,ryy,rzz);
				commandSource=joyStickControl;
			}
		else if(pSerialReceiveBuffer[0] == 'h')
			{

				int holdflagtemp[BELLOWNUM];
				int retNum=sscanf(&pSerialReceiveBuffer[2], "%d %d %d %d %d %d",&holdflagtemp[0],&holdflagtemp[1],&holdflagtemp[2],&holdflagtemp[3],&holdflagtemp[4],&holdflagtemp[5]);
				if (retNum == -1 )
					for (int i=0;i<BELLOWNUM;i++)
						holdflag[i]=1;
				else
					for (int i=0;i<retNum;i++)
						holdflag[holdflagtemp[i]]=1;
			}
		else if(pSerialReceiveBuffer[0]=='l')
			{
				int holdflagtemp[BELLOWNUM];
				int retNum=sscanf(&pSerialReceiveBuffer[2], "%d %d %d %d %d %d",&holdflagtemp[0],&holdflagtemp[1],&holdflagtemp[2],&holdflagtemp[3],&holdflagtemp[4],&holdflagtemp[5]);
				if (retNum == -1 )
					for (int i=0;i<BELLOWNUM;i++)
						holdflag[i]=0;
				else
					for (int i=0;i<retNum;i++)
						holdflag[holdflagtemp[i]]=0;
			}

}



void SOFT_ARM::increaseFrequency(float dFre){
	frequency = frequency+dFre;
	frequency = CONSTRAIN(frequency,0.05,10000);
	for (int i = 0; i < BELLOWNUM; i++) {
		basePlatform->chambers[i]->valves[0].writeFrequency(frequency);
		basePlatform->chambers[i]->valves[1].writeFrequency(frequency);
	}
}
void SOFT_ARM::increaseVelocity(float dVel)
{
	openingBase = openingBase+dVel;
}

float SOFT_ARM::readPressure(int num)
{
	pressure[num]=basePlatform->chambers[num]->readPressure();
	return pressure[num];
}
void SOFT_ARM::readPressureAll()
{
	for (int i = 0; i < BELLOWNUM; i++) {
		pressure[i]=basePlatform->chambers[i]->readPressure();
	}
}

void SOFT_ARM::writeOpeningAll(float op)
{
	for (int i = 0; i < BELLOWNUM; i++) {
		basePlatform->chambers[i]->writeOpening(op);
	}
}

void SOFT_ARM::controlPressureAll()
{
	for (int i = 0; i < BELLOWNUM; i++) {
		/*************Holding flag would override the control outcome***********/
		if(holdflag[i] == 1){
			basePlatform->chambers[i]->writeOpening(0);
		}
		else
		{
			pressureCommand[i]=CONSTRAIN(pressureCommand[i],pressureCommandMin[i],pressureCommandMax[i]);
			basePlatform->chambers[i]->writePressure(pressureCommand[i]);
		}
	}
}
void SOFT_ARM::writePressure(int num,float pre)
{
	pressureCommand[num]=pre;
	pressureCommand[num]=CONSTRAIN(pressureCommand[num],pressureCommandMin[num],pressureCommandMax[num]);

}

void SOFT_ARM::joyStickController(int joystickMode)
{
	/******************Buttons*****************************/
		//take care of button 2 every 1ms

		// mode = joyStick.BtnThumbL;
		 joyStick.jx = ((joyStick.jx < Rmin && joyStick.jx > -Rmin)? 0 : (CONSTRAIN(joyStick.jx,-Rmax,Rmax)));
		 joyStick.jy = ((joyStick.jy < Rmin && joyStick.jy > -Rmin)? 0 : (CONSTRAIN(joyStick.jy,-Rmax,Rmax)));

		 //only check button 1  every 100ms
		if (buttonCheckTime[1]-- <= 0)
		{
			if (joyStick.jz>100) {
				if(0.05<=frequency && frequency<0.5)
					increaseFrequency(0.05);
				else if(frequency<1)
					increaseFrequency(0.1);
				else if(frequency<50)
					increaseFrequency(1);
				else if(frequency<200)
					increaseFrequency(2);
				else if(frequency<2000)
					increaseFrequency(10);
				else if(frequency<10000)
					increaseFrequency(100);
			}
			else if(joyStick.jRz>100)
			{
				if(0.05<frequency && frequency<=0.5)
					increaseFrequency(-0.05);
				else if(frequency<=1)
					increaseFrequency(-0.1);
				else if(frequency<=50)
					increaseFrequency(-1);
				else if(frequency<=200)
					increaseFrequency(-2);
				else if(frequency<=2000)
					increaseFrequency(-10);
				else if(frequency<=10000)
					increaseFrequency(-100);
			}
			buttonCheckTime[1] = 20;
		}

		//no single button pressed
		if(!(joyStick.BtnEast || joyStick.BtnNorth || joyStick.BtnSouth || joyStick.BtnWest || joyStick.BtnThumbL || joyStick.BtnThumbR))
			{
			/******************Jx Jy*****************************/
			//dead zone and saturation
			 joyStick.jRx = ((joyStick.jRx < Rmin && joyStick.jRx > -Rmin)? 0 : (CONSTRAIN(joyStick.jRx,-Rmax,Rmax)));
			 joyStick.jRy = ((joyStick.jRy < Rmin && joyStick.jRy > -Rmin)? 0 : (CONSTRAIN(joyStick.jRy,-Rmax,Rmax)));

			if(joystickMode==0){

				//opening base for elongation and contraction
				if(joyStick.jy>0)
								 openingBase = 0.5; //elongation
							 else if(joyStick.jy<0)
								 openingBase = -0.8; //contraction
							 else
								 openingBase = 0; //rotation

				//additional opening for rotation
				if(joyStick.jRx==0 && joyStick.jRy == 0){
					rawAngle=0;
					rawAmplitude=0;
					rawAmplitudeMax = Rmax;
				}
				else{
					rawAngle = atan2f((float)joyStick.jRx, (float)joyStick.jRy);   //(-pi~pi)
					arm_sqrt_f32((float)(joyStick.jRx * joyStick.jRx + joyStick.jRy * joyStick.jRy),&(rawAmplitude));

					if (0< rawAngle)
					{
						if( rawAngle < M_PI_4 || rawAngle > M_3PI_4)
							rawAmplitudeMax = Rmax / arm_cos_f32(rawAngle);
						else
							rawAmplitudeMax = Rmax / arm_sin_f32(rawAngle);
					}
					else
					{
						if (-M_PI_4 < rawAngle || rawAngle < -M_3PI_4)
							rawAmplitudeMax = Rmax / arm_cos_f32(rawAngle+M_PI);
						else
							rawAmplitudeMax = Rmax / arm_sin_f32(rawAngle+M_PI);
					}
				}
				angleCommand = rawAngle;
				amplitudeCommand = fabsf(rawAmplitude/rawAmplitudeMax);
				for (int i = 0; i < BELLOWNUM; i++) {
					bellowProjection[i] = (arm_cos_f32(angleCommand)* bellowConfigurationPositionX[i] + arm_sin_f32(angleCommand) * bellowConfigurationPositionY[i]);
					basePlatform->chambers[i]->writeOpening(-(bellowProjection[i]/bellowConfigurationRadius * amplitudeCommand*0.5) + openingBase);
					}
			}
			else if(joystickMode==1){
				//pressure base for elongation and contraction
				if(joyStick.jy>0)
					 pressureBase += 50; //elongation
				 else if(joyStick.jy<0)
					 pressureBase += -50; //contraction

				//additional pressure for rotation
				angleCommand += ((float)joyStick.jRy)/Rmax*0.01;   //(-1 degree ~1 degree)
				if(angleCommand>M_PI)
					angleCommand-=2*M_PI;
				else if(angleCommand<=-M_PI)
					angleCommand+=2*M_PI;

				amplitudeCommand += ((float)joyStick.jRx)/Rmax*0.01;
				amplitudeCommand=CONSTRAIN(amplitudeCommand,0,M_PI/2);

				controlPressureAll();
			}
		}
		 else {
			 int individualChoosenFlag[6]={0,0,0,0,0,0};
			 if (joyStick.BtnSouth)
				 individualChoosenFlag[0]=1;
			 if(joyStick.BtnEast)
				 individualChoosenFlag[1]=1;
			 if(joyStick.BtnNorth)
				 individualChoosenFlag[2]=1;
			 if(joyStick.BtnWest)
				 individualChoosenFlag[3]=1;
			 if(joyStick.BtnThumbR)
				 individualChoosenFlag[4]=1;
			 if(joyStick.BtnThumbL)
				 individualChoosenFlag[5]=1;
			 for(int i=0;i<BELLOWNUM;i++)
				 if(individualChoosenFlag[i])
					 basePlatform->chambers[i]->writeOpening(openingBase);
		 }

}

void SOFT_ARM::switchValveStatus()
{
	int inFlag=0;
	int outFlag=0;
	for(int i=0;i<BELLOWNUM;i++){
		 if(basePlatform->chambers[i]->opening>0){
			 inFlag=1;
			 continue;
		 }
		 else if(basePlatform->chambers[i]->opening<0){
			 outFlag=1;
			 continue;
		 }
	}
	if(inFlag)
		basePlatform->pSource.pump.start();
	else
		basePlatform->pSource.pump.stop();
	if(outFlag)
		basePlatform->pSink.pump.start();
	else
		basePlatform->pSink.pump.stop();
}

float SOFT_ARM::readLength() {
	float vol=AnalogRead(lengthAnalogPort);
	length= MAP(vol,0,10.0,0,1);
	return length;
}

void SOFT_ARM::readIMU(char *pBuffer) {
	IMUFRAME *pFrame=(IMUFRAME *)pBuffer;
	if(pFrame->headerEuler[0]==0x55 && pFrame->headerEuler[1]==0x53)
	{
		imuData.angleX=(pFrame->Rx)/32768.0f*180;
		imuData.angleY=(pFrame->Ry)/32768.0f*180;
		imuData.angleZ=(pFrame->Rz)/32768.0f*180;
		imuData.q0=(pFrame->q0)/32768.0f;
		imuData.q1=(pFrame->q1)/32768.0f;
		imuData.q2=(pFrame->q2)/32768.0f;
		imuData.q3=(pFrame->q3)/32768.0f;
	}
}


void SOFT_ARM::lengthTrack(float lengthD)
{

	for(int i=0;i<BELLOWNUM;i++)
		pressureCommand[i]= k0*(lengthD-length0)/crossA;
}
void SOFT_ARM::alphaTrack(float alphaD)
{

}
void SOFT_ARM::betaTrack(float betaD)
{

}
void SOFT_ARM::stiffnessAlphaTrack()
{

}



