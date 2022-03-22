#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <grx20.h>
#define CD 0.5      	// Assumed value of drag coefficient - impossible to calculate non-experimentally. 
#define AREA 22.9   	// Reference area of the front of the rocket 
#define G0 9.80665      // Gravitational field strength at surface 
#define RADIUS 6.371E6  // Radius of Earth


typedef struct{
    float t[2501];  	// time 
	float v[2501];  	// velocity 
	float a[2501];  	// acceleration
	float h[2501];  	// altitude
	float ft[2];    	// thrust of centaur engine (ft[0]) and solid boosters (ft[1]) (kN) (each)
	float mc;         // Common centaur mass
	float m[2];       // solid mass of centaur engine and adapter (m[0]) and solid boosters (m[1])  (each)
	float f[2];        	// mass of fuel in atlas booster (f[0]) and solid boosters (f[1]) (each)
	float p;        	// payload mass
	int nsrb;		// number of SRBs
	int nab;        // number of atlas boosters;

} Variables;

float calcGravityForce (Variables var, int i, float totalmass), calcDragForce (Variables var, int i), useFuel(Variables var, int isSRB),
getAirDens(Variables var, int i), calcTemp(Variables var, int i), calcPress(Variables var, int i);
void writeFile(Variables var), writecsvFile (Variables var), newSelection(), startPara(Variables var), startMenu(Variables var), resultsMenu (Variables var), makeTable(Variables var), graphMenu (Variables var), graphFunc(float y[], float x[], char ylable[], int colour);
Variables changeMenu(Variables var), readFile(Variables var);

Variables initialise()
{
   	
	Variables var;
	/* Assigning default values */
	var.v[0] = 0.0;
	var.h[0] = 0.0;
	var.a[0] = 0.0;
	var.t[0] = 0.0;
    var.nsrb = 3;
    var.nab = 1;
	var.ft[0] = 3827.0;
	var.ft[1] = 1688.4;
	var.mc = 20570.0;
	var.m[0] = 23863.0;
	var.m[1] = 4069.0;
	var.f[0] = 284089.0;
	var.f[1] = 42630.0; 		// From Atlas V data sheet: http://www.spacelaunchreport.com/atlas5.html#config
	var.p = 0.0;
	
	return var;
	}
	
int main() {
    Variables var;
    var = initialise();
	startMenu(var);	//Run user menu
}
	
Variables runSimulation(Variables var) {
    int i, srbIsDetached;
    float totalmass;
    Variables simul = var;          // Make a copy of var for the simulation, so that the changing mass and thrust over time doesn't change the original values.
    srbIsDetached = 0;            	// Solid rocket boosters have not been detached
	for(i = 1; i<=2500; i++) {     	
        totalmass = var.mc + var.nab*(var.m[0] + var.f[0]) + (1-srbIsDetached)*var.nsrb*(var.m[1]  + var.f[1]) + var.p; // The total mass of the rocket, taking into account the number of atlas booster engines and SRBs

        simul.t[i] = (float)(floor( (simul.t[i-1] + 0.1) * 10 + 0.5 ) / 10);  // increase time elapsed by increments of 0.1s
        simul.a[i-1] = (1000*(var.nab*var.ft[0] + var.nsrb*(1-srbIsDetached)*var.ft[1]) - calcGravityForce(var, i-1, totalmass) - calcDragForce(var, i-1)) / totalmass;

    	simul.v[i] = simul.v[i-1] +simul.a[i-1]*0.1;
   		simul.h[i] = simul.h[i-1] + simul.v[i]*0.1;
   		var.h[i] = simul.h[i];   // These affect temp, pressure and air resistance so must be assigned to var as well as simul so it can be used in the calculation functions
   		var.v[i] = simul.v[i];
           
   		if(i==1150)  srbIsDetached = 1;

		if(var.f[0] != 0) var.f[0] = useFuel(var, 0);
		if(!srbIsDetached && var.f[1] != 0) var.f[1] = useFuel(var, 1);

		if(var.f[0] == 0) var.ft[0] = 0;  // If there is no fuel, reduce the thrust from that engine to 0
		if(var.f[1] == 0) var.ft[1] = 0;
	} 
simul.a[2500] = (1000*(var.nab*var.ft[0] + var.nsrb*(1-srbIsDetached)*var.ft[1]) - calcGravityForce(var, 2499, totalmass) - calcDragForce(var, 2499)) / totalmass;
printf("Simulation complete.\n");
return simul;
}

float calcGravityForce (Variables var, int i, float totalmass) {
           return totalmass * G0 * pow(RADIUS/(RADIUS+var.h[i]),2.0);
	// Using F = mg where g = g0*(r/(r+h))^2 
}

float calcDragForce (Variables var, int i) {
  	return 0.5 * getAirDens(var, i) * var.v[i]*var.v[i] * CD * AREA;
	// Using assumed value of Cd given above
}


float useFuel(Variables var, int isSRB) { // reduce the mass of fuel by (fuel consumption) * (time period) 
	float consumption, isp;
isp = isSRB ? 279.3 : 337.8;		// isp of SRB and Atlas booster (assuming vacuum and 100% thrust)
consumption = 100 * var.ft[isSRB] / (isp * G0);
	
	if(var.f[isSRB] < consumption)  return 0;      // If there is insufficient fuel, return 0
	else return var.f[isSRB] - consumption;
}


float getAirDens(Variables var, int i){
return calcPress(var, i)/(0.2869*(calcTemp(var, i)+273));
//using Air Density = Air Pressure / (0.2869 * (Temperature+273))
//http://www.grc.nasa.gov/WWW/k-12/airplane/atmosmet.html
}

void writeFile(Variables var) {
	int i;
	FILE *fp;
	fp = fopen("rocketData.txt", "w");
	fprintf(fp,"%d\t%d\t%f\t%f\n",var.nsrb,var.nab,var.f[0],var.ft[0]); // Saving all the parameters that are able to be changed
	/* for(i = 0; i<2500; i++) {
    	fprintf(fp, "%2.1f\t%6.3f\t%6.3f\t%6.3f\n", var.t[i], var.a[i], var.v[i], var.h[i]);        -- Storing the a,v,t,h values is unneccessary as the simulation runs automatically when the resultsMenu() opens
	} */
	fclose(fp);
}

Variables readFile(Variables var) {  // Read data from the file
    FILE *fp;
    fp = fopen("rocketData.txt", "r");
    if(fp == NULL) printf("No such file\n");
    fscanf(fp,"%d\t%d\t%f\t%f\n",&var.nsrb,&var.nab,&var.f[0],&var.ft[0]);
    fclose(fp);
    return var;
}

void writecsvFile (Variables var) {
    int i;
    FILE *fp;
    fp = fopen("rocket_simulation_results.csv", "w");
    fprintf(fp,"Time, Acceleration, Velocity, Altitude\n");
    for(i = 0; i<2500; i++) {
    	fprintf(fp, "%2.1f, %6.3f, %6.3f, %6.3f\n", var.t[i], var.a[i], var.v[i], var.h[i]);
    }
    fclose(fp);
    printf("Results written to file \"rocket_simulation_results.csv\"\n");
}

float calcTemp(Variables var, int i){
	float h = var.h[i];
	if(h<=11000)
		return 15.04-(0.00649*h);
	if(11000 < h && h <=25000)
		return -56.46;
	if(h>25000)
		return -131.21+0.0029*h;
}

float calcPress(Variables var, int i){
float h = var.h[i];
 if(h<=11000)
  return 101.29*pow((calcTemp(var,i)+273.0)/288.08,5.256);
 if(11000<h && h<=25000)
  return 22.65*pow(2.7183,1.73-0.000157*h);
  /* 2.7183 is an approximation of e */
 if(h>25000)
  return 2.488*pow((calcTemp(var,i)+273)/216.6,(-11.388));
}

void startMenu(Variables var){ //first user menu
    int o, exit;	//ints used in menu navigation
    exit = 0;
    //display options
    printf("A program to simulate the first 250 seconds of the launch of an Atlas V 500 series launch system\n");
    printf("Please select an option from the menu, by entering the appropriate number\n");
    while (!exit) {
        printf("\n1.View the current system starting parameters\n");
        printf("2.Change the starting system parameters\n");
        printf("3.Run simulation and view program results\n");
        printf("4.Load previous simulation data\n");
        printf("5.Exit program\n");
        scanf("%d",&o);	//user enters selection
        if(o == 1)		//check user entry
            startPara(var);	//Launch menu to view starting parameters
        else if(o == 2)
		    var = changeMenu(var);		//Launch menu to change starting parameters	
        else if(o == 3)	
	        resultsMenu(var);		//Launch menu to select how to view results
        else if (o == 4) 
            var = readFile(var);
        else if (o == 5) exit = 1;
        else printf("That is an invalid entry, please try again\n"); // if user inputs invalid entry
    }
}

void startPara(Variables var){	//Menu to view starting parameters
    int x, z;
	//Choose parameter to view
	printf("\nPlease select the parameter's starting value that you would like to see, by entering the appropriate number\n");
	printf("1.Mass of liquid fuel\n");
	printf("2.Mass of rocket without liquid fuel and Solid Rocket Boosters\n");
	printf("3.Number of Solid Rocket Boosters\n");
	printf("4.Total mass of Solid Rocket Boosters\n");
	printf("5.Satellite carried\n");
	printf("6.Mass of satellite carried\n");
	printf("7.Total mass of the rocket\n");
	printf("8.Number of atlas booster engines\n");
    printf("9.Launch altitude\n"); 
	printf("10.Total thrust at liftoff\n");
	printf("11.View all parameters at once\n");
	printf("Please enter 12 if you wish to return to the previous menu\n");
	while(1) {
	scanf("%d",&x);	//User select
	if(x == 12) return;
	if(x!=11) z = x;
	else z = 0;
	printf("\n");
	do{
	if(x==11) z++; // If x == 11 loop through all cases.
	switch (z) { //Check User selection
        case 1:	
	        printf("The current mass of liquid fuel is %6.1f\n", var.f[1]);
	        break;
        case 2:	
		    printf("The current mass of rocket without liquid fuel and Solid Rocket Boosters is %6.1f\n", var.mc + var.nab * var.m[0]);
		    break;
        case 3:		
	        printf("The current number of Solid Rocket Boosters is %d\n", var.nsrb);
	        break;
        case 4:		
		    printf("The current total mass of Solid Rocket Boosters is %6.1f\n", var.nsrb*(var.m[1] + var.f[1]));
		    break;
        case 5:	
            printf("Not yet implemented\n");
		    /* printf("The current satellite carried is %s", ); */
		    break;
        case 6:		
		    printf("The current mass of satellite carried is %6.1f\n", var.p);
		    break;
        case 7:		
		    printf("The current total mass of the rocket is %6.1f\n", var.mc + var.nab*(var.m[0] + var.f[0]) + var.nsrb*(var.m[1] + var.f[1]) + var.p);
		    break;
        case 8:	
	        printf("The current number of atlas booster engines is %d\n", var.nab);
	        break;
        case 9:	
		    printf("The current launch altitude is %6.1f\n", var.h[0]);
		    break;
        case 10:	
		    printf("The total thrust at liftoff is %6.1f\n", var.nab*var.ft[0] + var.nsrb*var.ft[1]);
		    break;
        case 11:
            x = 1; // x is no longer equal to 11, so the do ... while loop ends
            break;
        default:
            printf("Invalid choice\n");   //if user inputs invalid entry
            break;
            }
		} while (x==11);
		printf("Please select another option from the menu above\n");
		
}
}


Variables changeMenu(Variables var){
    int x, n;
    float y;
    while (1) {
	//Choose parameter to change
	printf("\nPlease select a starting parameter to change by entering the appropriate number\n");
	printf("1.Mass of liquid fuel in atlas booster\n");
	printf("2.Number of Solid Rocket Boosters\n");
	printf("3.Number of centaur engines\n");
	printf("4.Launch altitude\n");
	printf("5.Thrust of atlas booster\n");
	printf("Please enter 6 if you wish to return to the previous menu\n");
	scanf("%d",&x); //check user input
	if(x == 1) {
		printf("\nPlease enter a value, in kilograms, between 100 000 and 1 000 000\n");
		scanf("%f",&y);
        if(100000<=y && y<=1000000) //basic error checking
        var.f[0] = y;
        else printf("Sorry, that entry was invalid please try again\n"); 
    }
    else if(x == 2) {
	    printf("\nPlease enter an integer between 0 and 5 inclusive\n");
	    scanf("%d", &n);
	    if(0<=n && n<=5)
		    var.nsrb = n;
        else printf("Sorry, that entry was invalid please try again\n");
    }
    else if(x == 3) {
	    printf("\nPlease enter an integer between 1 and 3 inclusive\n");
	    scanf("%d", &n);
	    if(1<=n && n<=3)
		    var.nab = n;
        else printf("Sorry, that entry was invalid please try again\n");
    }
    else if(x == 4) {
	    printf("\nPlease enter a value, in metres between 0 and 8000\n");
	    scanf("%f", &y);
	    if(0<=y && y<=8000)
		    var.h[0] = y;
        else printf("Sorry, that entry was invalid please try again\n");
    }
    else if(x == 5) {
        printf("\nPlease enter a value in kN between 1000 and 100 000\n");
        scanf("%f",&y);
        if(1000<=y && y<=100000)
		    var.ft[0] = y;
        else printf("Sorry, that entry was invalid please try again\n");
    }
    else if(x == 6)
	    return var;
    else printf("That is an invalid entry, please try again\n"); //if user inputs invalid entry
}
}

void resultsMenu(Variables var) {
    int x;
    var = runSimulation(var);
    while(1) {     
        printf("\n1.Graph\n");
        printf("2.Table\n");
        printf("3.Save configuration\n");
        printf("4.Write results to csv file\n");
        printf("Please enter 5 to return to the previous menu\n");

        scanf("%d",&x);
        switch (x) {
            case 1:
                graphMenu(var);
                break;
            case 2:
                makeTable(var);
                break;
            case 3:
                writeFile(var);
                break;   
            case 4:
                writecsvFile(var);
                break;
            case 5:
                return;
            default:
                printf("Invalid choice\n");   
                break;   
        }
    }
}

void graphMenu(Variables var) {
    int x;
    while(1) {
        printf("Please selct the variable you want plotted against time by selcting the appropriate number \n");        
        printf("1.Displacement\n");
        printf("2.Velocity\n");
        printf("3.Acceleration\n");
        printf("Please enter 4 to return to the previous menu\n");

        scanf("%d",&x);
        switch (x) {
            case 1:
                graphFunc(var.h,var.t,"Displacement", 1);
                break;
            case 2:
                graphFunc(var.v,var.t,"Velocity", 2);
                break;
            case 3:
                graphFunc(var.a,var.t,"Acceleration", 4);
                break;   
            case 4:
                return;
            default:
                printf("Invalid choice\n");   
                break;   
        }
    }
}

void graphFunc(float y[2501], float t[2501], char ylable[], int colour) {
    int i, xres, yres, x;
    float tmin, tmax, ymin, ymax, tnew, ynew, told, yold;
    tmin = 0;
    tmax = 0;
    ymin = 0;
    ymax = 0;
    for(i = 0; i<=2500; i++) {
         if( t[i] < tmin)
             tmin = t[i];
         if( t[i] > tmax)
             tmax = t[i];
    }
       for(i = 0; i<=2500; i++) {
          if( y[i] < ymin) 
               ymin = y[i];
          if( y[i] > ymax) 
               ymax = y[i];
    }      
    GrSetMode(GR_default_graphics); 
    GrClearScreen(15);
    xres = GrScreenX();
    yres = GrScreenY();
    for(i = 0; i<=2500; i++) {
           tnew = (20 + (t[i] - tmin) * ((xres - 40) / (tmax - tmin)));
           ynew = yres - (20 + (y[i] - ymin) * ((yres - 40) / (ymax - ymin)));
           GrCircle(tnew,ynew,1,colour); 
           if(i>0) GrLine(told,yold,tnew,ynew,colour);
           told = tnew;
           yold = ynew;
    }
    GrLine(20, 20, 20, yres - 20, 0);
    GrLine(20, yres - 20 + ymin, xres - 20, yres - 20 + ymin, 0);
    GrTextXY(xres / 2,yres - 15,"time", 1, 15); 
    GrTextXY(10,20,ylable, 1, 15); 
    printf("Please press 1 to return\n");
    x=0;
    while(x!=1) scanf("%d",&x);
    GrSetMode(GR_default_text);
}

void makeTable(Variables var) {
    int i, increment, x;
    printf("What precision would you like?\n");
    printf("1. 0.1s increments\n");
    printf("2. 0.5s increments\n");
    printf("3. 1s increments\n");
    scanf("%d", &x);
    if(x==1) increment = 1;
    else if(x==2) increment = 5;
    else if(x==3) increment = 10; 
    else {
        printf("Invalid choice\n");
        makeTable(var);
        return;
    }
    printf("t\ta\tv\th\n");
    for(i = 0; i<=2500; i+= increment) {
    	printf("%2.1f\t%6.3f\t%6.2f\t%6.2f\n", var.t[i], var.a[i], var.v[i], var.h[i]);        
	}
	for(i=0;i<1;) {
	    printf("\n Please press 1 to return \n");
	    scanf("%d",&x);
    }
}
