// Delivery Optimisation

// Program description and information.
/*

Program used to find the optimal location in great britain to place a logistical hub. The program optimises such tha the cost for delivery is minimised. It does this by
taking in a CSV file containing location and population data for the top 100 most populous cities in great britain, and using this to calculate the location.

The Assumptions made are the following:
 - The cost of delivering to a location is well modelled by the distance to this location weighted by some value (more to follow)
 - The distance to a location is simply the Great Circle Distance to that location across the earth, which is a sphere.
 - The weight on a location is the population of the location, as statistically this will increase the demand for products.
 - The weight for cities is twice that for towns, since the economic activity in cities is greater than that in towns, further increasing the demand for products.
 - Only a single hub is needed for the entirety of Great britain.
 - Only the 100 most populated locations matter for the model.

 The program optimises using a hill climbing method, with many randomised starting locations in order to attempt to find the global minima of the cost,
 rather than simply a local minima.

 After optimising for a Hub location, the program asks if the user would like to produce a Minimum Spanning Tree for the locations in the data set.
 If so, the program uses Prim's algorithm to produce a MST, with the weight between vertices being the Great Circle Distance, halved in the case of cities.
 This minimum spanning tree could be used in order to produce efficient delivery routes between the locations from the hub location, which is always the start
 point for the tree.

 Hub location consistently found at (52.2674, -1.10403) with a run time of approximately 20 seconds.

*/

// Preliminaries

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <time.h>// Included features

using namespace std; // using standard library

double PI = 3.1415; // definition of pi.

// Data Structures

struct LocData{
	string name;
	string type;
	int pop;
	double lat;
	double lon;
};// Data point for a single Population centre, containing all the data for a single population point.

struct climber{
	double Lat;
	double Lon;
	// Latitude and longitude for the climbers current postion.

	double cost;
	// current cost value for the climber.
	
	bool PositiveLat;
	bool PositiveLon;
	// Boolean statements for if the next iteration should increase latitude and longitude

	double MoveWeightX;
	double MoveWeightY;
	// Defines a move weight for the next climbing iteration.
	
	int Iteration;
	// Counts the number of iterations of climbing.
};// Structure for a climbing entity to be used in hill climbing.

struct Edge{
	LocData Node1, Node2;
	int VecLoc1, VecLoc2;
	double Weight;
};// Data structure for an edge in a graph.

struct Tree{

	vector<LocData> Vertices;
	// Vector containing all the vertices contained within the tree.

	vector<Edge> TreeEdges;
	// Vector containing all the edges contained within the tree.
	
	double Weight;
	// Weight of the tree.

};// Data structure describing a Graphical Tree.

struct UnSpanned{

	vector<LocData> Data;
	// Data contained within the structure

	vector<bool> Spanned;
	// vector of Booleans of if an index is spanned.

};// Structure to contain data while showing which of these points is already within the tree.

// Function Definitions

vector<vector<string>> FileReader(string FileName){
	// Function reads in a CSV file to a 2D vector with all elements as strings.

	vector<vector<string>> Data;
	vector<string> IntermediaryVector;
	vector<int> commas;
	string Line, SubString;
	// Declare vaiables.

	ifstream FileData(FileName);
	// Opens the file for reading.

	if (FileData.is_open()){
		// Checks if the file is open

		while (!FileData.eof()){
			getline(FileData, Line);
			// Get a line while not at the end of the file.

			if (Line.length() > 0){
				// Check if the line contains anything.

				commas.push_back(Line.find(',', 0));
				SubString = Line.substr(0, commas[0]);
				IntermediaryVector.push_back(SubString);
				// Find the first comma in the Line, then create a substring of the contents before this comma, 
				// and append this to the intermediary vector.

				int i = 0;
				// Declare a counting variable

				while (commas[i] != -1){
					// While not at the end of the line,

					commas.push_back(Line.find(',', commas[i] + 1));
					// Finds the next comma in the line

					if (commas[i + 1] != -1){
						SubString = Line.substr(commas[i] + 1, commas[i + 1] - commas[i] - 1);
					}// If not at end of the line, creates a sub string of contents between this next comma and and the one previously.

					else{
						SubString = Line.substr(commas[i] + 1, Line.length() - commas[i] - 1);
					}// If at end of the line, creates a sub string of the contents at the end of the line.

					IntermediaryVector.push_back(SubString);
					// Appends the substring to the intermediary vector.

					i++;
					// Increases counter variable.
				}

				Data.push_back(IntermediaryVector);
				// Appends the intermediary vector to the matrix once the line is finished reading.

				IntermediaryVector.clear();
				commas.clear();
				// Clears the vectors for reuse.
			}
		}

		FileData.close();
		cout << "File correctly read in! :)  \n";
		// Closes the file once reading is done, tells user of this.
	}

	else{
		cout << "Sorry, the file could not be opened correctly, please check the name and location of the file are correct and try again.  \n";
	}// If file fails to open, notify the user of this.

	return Data;
}

double Haversine(double Angle){
	double Hav = pow(sin(Angle / 2), 2);
	return Hav;
}// Calculates the haversine for an angle

double GCD(LocData Place1, LocData Place2){
	// Function determines the Great Circle Distance (GCD) between 2 locations.
	
	double R = 6378.1;
	// Radius of Earth in km.
	
	double HavTheta = Haversine(Place1.lat - Place2.lat) + cos(Place1.lat)*cos(Place2.lat)*Haversine(Place1.lon - Place2.lon);
	// Haversine of the great circle distance between 2 points.
	
	double D = 2 * R*asin(sqrt(HavTheta));
	// Calculates The Great circle distance between the 2 places. 

	return D;
}

double WeightDistSum(climber C, vector<LocData> Data){
	// Perform a wighted sum of the great circle distances between a climber location and 
	// the locations of the various cities/towns, using the population as the weight.
	
	double X = 0;
	// Declare initial value to add to.

	LocData Cloc;
	Cloc.lat = C.Lat;
	Cloc.lon = C.Lon;
	// Put LatLon values in to correct type to use in GCD formula
	
	for (int i = 0; i < Data.size(); i++){
		if (Data[i].type == "Town"){
			X = X + Data[i].pop*GCD(Cloc, Data[i]);
		}
		else if (Data[i].type == "Town"){
			X = X + 2 * Data[i].pop*GCD(Cloc, Data[i]);
		}// Checks if the place is a town or a city, gives double weight to cities due to the extra economic activity of these locations.
	}// Sum up GCD's between climber and locations, multiplied by the population at this location.

	return X;
}

double DTR(double DAngle){
	double RAngle = DAngle*(PI / 180);
	return RAngle;
}// Converts from degrees to radians

double RTD(double RAngle){
	double DAngle = RAngle*(180 / PI);
	return DAngle;
}// Converts from radians to degrees

double RandonNumber(double max, double min, int mod){
	double R = min + (rand() % (mod + 1) *(1. / mod)*(max - min));
	return R;
}// Finds a random numbber between a maximum and minimum value, with a specified modulus.

// Main Program

int main(){

	srand(time(NULL));
	// Seed RNG using Computer clock.

	string FileName = "GBplaces.csv";
	// Defines file name as 'GBplaces.csv'. Program can in theory be used for any size data set of the same format as GBplaces.csv.
	
	vector<vector<string>> Data = FileReader(FileName);
	vector<LocData> DataFormatted;
	LocData IntermediaryLocData;
	// Declare variables and read in data.

	for (int c = 1; c < Data.size(); c++){
		IntermediaryLocData.name = Data[c][0];
		IntermediaryLocData.type = Data[c][1];
		IntermediaryLocData.pop = atoi(Data[c][2].c_str());
		IntermediaryLocData.lat = DTR(atof(Data[c][3].c_str()));
		IntermediaryLocData.lon = DTR(atof(Data[c][4].c_str()));
		DataFormatted.push_back(IntermediaryLocData);
	}// Creates a intermediary data point from read in data, making sure types are correct and 
	 // converting degrees to radians, then appends to a vector. Starts at 1 in order to remove column headers.

	vector<climber> Climbers;
	climber IntermediaryClimber;
	for (int i = 0; i < 80; i++){
		IntermediaryClimber.Lat = DTR(RandonNumber(58, 50, 1000));
		IntermediaryClimber.Lon = DTR(RandonNumber(2, -6, 1000));
		IntermediaryClimber.MoveWeightX = 1;
		IntermediaryClimber.MoveWeightY = 1;
		IntermediaryClimber.Iteration = 0;
		IntermediaryClimber.cost = WeightDistSum(IntermediaryClimber, DataFormatted);
		Climbers.push_back(IntermediaryClimber);
	}// Creates a vector of 50 climbers in random locations, in order to find the global minimum. 
	 // Initial moveweight is set to 1.

	double CostDX, CostDY;
	// Declare doubles for change in cost.

	for (int i = 0; i < Climbers.size(); i++){
		while (Climbers[i].Iteration < 120){
		// For each climber, While not at maximum number of specified iterations.

			IntermediaryClimber.Lat = Climbers[i].Lat + (0.001);
			IntermediaryClimber.Lon = Climbers[i].Lon;
			CostDY = WeightDistSum(IntermediaryClimber, DataFormatted);
			// Calculate cost of climber location with slight pertubation to the north.

			IntermediaryClimber.Lat = Climbers[i].Lat;
			IntermediaryClimber.Lon = Climbers[i].Lon + (0.001);
			CostDX = WeightDistSum(IntermediaryClimber, DataFormatted);
			// Calculate cost of climber location with slight pertubation to the east.

			Climbers[i].PositiveLat = (CostDY > Climbers[i].cost);
			Climbers[i].PositiveLon = (CostDX > Climbers[i].cost);
			Climbers[i].MoveWeightY = 15*abs((CostDY-Climbers[i].cost) / Climbers[i].cost);
			Climbers[i].MoveWeightX = 15*abs((CostDX-Climbers[i].cost) / Climbers[i].cost);
			// Determine if cost increases in positive latitude and longitude directions, 
			// and determine move weights to be used from the ratio of costs.

			if (Climbers[i].PositiveLat){
				Climbers[i].Lat = Climbers[i].Lat - (0.05*Climbers[i].MoveWeightY);
			}else{
				Climbers[i].Lat = Climbers[i].Lat + (0.05*Climbers[i].MoveWeightY);
			}
			if (Climbers[i].PositiveLon){
				Climbers[i].Lon = Climbers[i].Lon - (0.05*Climbers[i].MoveWeightX);
			}else{
				Climbers[i].Lon = Climbers[i].Lon + (0.05*Climbers[i].MoveWeightX);
			}// Moves the climber according to to the direction which reduces the cost, with weighted values.
			
			Climbers[i].cost = WeightDistSum(Climbers[i], DataFormatted);
			// Calculate new cost of the climbers location.
			
			Climbers[i].Iteration++;
			// Increase the iteration number for the climber.
		}
	}

	climber BestClimb = Climbers[0];
	double BestCost = Climbers[0].cost;
	// Create a designation of best climber, with a best cost, with the first climber initially having this place.

	for (int i = 0; i < Climbers.size(); i++){

		if (Climbers[i].cost < BestCost){
			BestClimb = Climbers[i];
			BestCost = Climbers[i].cost;
		}// Check if the minima for the climber being checked is better than the previous best, if so, change the best tag.
	}

	BestClimb.Lat = RTD(BestClimb.Lat);
	BestClimb.Lon = RTD(BestClimb.Lon);
	// Converts values of the best climbers longitudes and latitudes to degrees for easy reading.

	cout << '\n' << "Best location is at   \n Lat: " << BestClimb.Lat << "\n Lon: " << BestClimb.Lon << "\n" << '\n';
	// prints the final best location.

	// From here, program finds a Minimum Spanning Tree (MST) using Prim's (Jarnik's) Algorithm for the location
	// data, starting from the hub, for use in finding efficient distribution routes. 

	string YN;
	cout << "Would you like to find a Minimum Spanning Tree for this Hub and Data set? \n";
	cout << "y/n? \n";
	cin >> YN;
	cout << '\n';
	// Asks the user if they would like a MST to be created.
	
	if (YN == "y" || YN == "Y" || YN == "Yes" || YN == "yes"){
	// If the user has answered yes in some form.

		UnSpanned UnSpanned;
		UnSpanned.Data = DataFormatted;
		for (int i = 0; i < UnSpanned.Data.size(); i++){
			UnSpanned.Spanned.push_back(false);
		}// Declare UnSpanned, set the data and set the spanned vector to false for all.

		Tree MST; 
		LocData MinLoc;
		double WeightedDistance;
		Edge BestEdge;
		BestEdge.Weight = pow(10, 10); // Set the initial best weight to an effectively infinite value.
		// Declare variables needed for the tree.

		LocData Hub;
		Hub.lat = DTR(BestClimb.Lat);
		Hub.lon = DTR(BestClimb.Lon);
		Hub.name = "Hub";
		MST.Vertices.push_back(Hub);
		// Create a Data point for the Hub, append this to the tree as an initial vertex.
		
		do{
			for (int i = 0; i < MST.Vertices.size(); i++){
				// For each vertex in tree
				for (int j = 0; j < UnSpanned.Data.size(); j++){
					// run over each unspanned vertex
					if (!UnSpanned.Spanned[j]){
						if (UnSpanned.Data[j].type == "Town"){
							WeightedDistance = GCD(MST.Vertices[i], UnSpanned.Data[j]);
						}
						else if (UnSpanned.Data[j].type == "City"){
							WeightedDistance = 0.5*GCD(MST.Vertices[i], UnSpanned.Data[j]);
						}// Find the Weighted Distance, which is simply either the GCD or half of that in the case of cities, to account for cities 
						 // being more economically active and better 'local hubs' for distribution to the surrounding area.

						if (WeightedDistance < BestEdge.Weight){
							BestEdge.Node1 = MST.Vertices[i];
							BestEdge.Node2 = UnSpanned.Data[j];
							BestEdge.Weight = WeightedDistance;
							BestEdge.VecLoc1 = i;
							BestEdge.VecLoc2 = j;
						}// If the index is spanned already, do nothing. 
					}
					WeightedDistance = pow(10, 10);
					// Set weighted distance to very high value for next iteration.
				}
			}

			MST.TreeEdges.push_back(BestEdge);
			MST.Vertices.push_back(BestEdge.Node2);
			MST.Weight = MST.Weight + BestEdge.Weight;
			// Appends the information of the best edge to the tree.
			
			UnSpanned.Spanned[BestEdge.VecLoc2] = true;
			// Set the spanned boolean for the newly added location to true so it does not get added again.
			
			BestEdge.Weight = pow(10, 10);
			// Set Weight of best edge to extremely high value so next iteration works.
			
		} while (MST.Vertices.size() < UnSpanned.Data.size()); // Perform loop until all locations are included in the tree.
		
		cout << "Minimum spanning tree for data set and hub location is: \n";
		for (int i = 0; i < MST.TreeEdges.size(); i++){
			cout << MST.TreeEdges[i].Node1.name << "--->>" << MST.TreeEdges[i].Node2.name << "\n";
		}// Prints the MST out to the user 1 edge at a time.
	}
	return 0;
}