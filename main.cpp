#include<iostream>
#include<fstream>
#include<ctime>
using namespace std;

#define INFINITY 10000
#define MAX_TABU_SIZE 100
#define MAX_ITERATION 1000
#define MAX_CAN_LIST 10000

//Global variable
int blocks,time_slots,peers,M;
int *upload_rate;
int *download_rate;
int **seeders;
int **costs;
//finish Global variable

struct node{
	int source;
	int iteration;
	int upload;
	int download;
	int cost;
	bool visited;
	bool leaf;
	node()
	{
		visited = false;
		upload = 0;
		download=0;
		leaf = true;
	}
};

struct Tabu{
	int aid;
	int bid;
	int asource;
	int bsource;
	bool swap;
	Tabu()
	{
		swap=true; // swap is true if its swap else add
	}
	Tabu(bool a)
	{
		swap = false;
	}
};

//print list and return the cost of the tree
void printList(node **list,ofstream &output)
{
	for(int j=0;j<peers;j++)
	{
		output << list[j]->cost <<" " <<list[j]->source <<" " << list[j]->iteration <<" " << list[j]->upload <<" " <<list[j]->download<<endl;
		//if(list[j]->cost != 0)
		//	output << "block " <<i+1 <<" is transferred from "<<list[j]->source+1 <<" to " << j+1<< " in iteration " <<list[j]->iteration<<endl;	
	}
}

//get ccost for the complete node
int getCost(node**list)
{
	int partial_cost=0;
	for(int j=0;j<peers;j++)
	{
		partial_cost += list[j]->cost;
	}
	return partial_cost;
}

//getCost for a candidate from candidate list
int getCost(int costA, int index, Tabu **candidateList)
{
	return costA - costs[candidateList[index]->aid][candidateList[index]->asource]
		- costs[candidateList[index]->bid][candidateList[index]->bsource]
		+ costs[candidateList[index]->aid][candidateList[index]->bsource]
		+ costs[candidateList[index]->bid][candidateList[index]->asource];
}

int getCost1(int costA, int index, Tabu **candidateList)
{
	return costA + costs[candidateList[index]->aid][candidateList[index]->asource]
		+ costs[candidateList[index]->bid][candidateList[index]->bsource]
		- costs[candidateList[index]->aid][candidateList[index]->bsource]
		- costs[candidateList[index]->bid][candidateList[index]->asource];
}

int getCostAdd(int costA, int index, Tabu **candidateList)
{
	return costA - costs[candidateList[index]->aid][candidateList[index]->asource]
		+ costs[candidateList[index]->aid][candidateList[index]->bsource];
}

int getCostAdd1(int costA, int index, Tabu **candidateList)
{
	return costA + costs[candidateList[index]->aid][candidateList[index]->asource]
		- costs[candidateList[index]->aid][candidateList[index]->bsource];
}

//get node ** list using greedy solution
node** Greedy(int block)
{
	node** list = new node*[peers];
	int count = 0;
	for(int i=0;i<peers;i++)
	{
		list[i] = new node();
		if(seeders[block][i] == 0)
		{
			list[i]->cost = INFINITY;
			list[i]->iteration = INFINITY;
			list[i]->source= INFINITY;
		}
		else
		{
			count++;
			list[i]->cost =0;
			list[i]->source=i;
			list[i]->iteration=0;
		}
	}	
	
	while(count < peers)
	{
		int min =0;
		while(list[min]->visited)
			min++;
		for (int i=0;i<peers;i++)
			if(list[min]->cost > list[i]->cost && !list[i]->visited)
				min = i;
		
		list[min]->visited = true;
		for(int i=0;i<peers;i++)
		{
			if(list[i]->cost == INFINITY && costs[min][i] != INFINITY && list[min]->upload < upload_rate[min]  && list[i]->download < download_rate[i])
			{
				list[min]->leaf = false;
				list[i]->cost = costs[min][i];
				list[i]->source = min;
				list[i]->iteration=list[min]->iteration+1;
				list[min]->upload++;
				list[i]->download++;
				count++;
			}
		}

	}
	return list;
}

//check for element in tabu list
bool containsTabuElements(int aid,int bid, int asource, int bsource, Tabu **tabuList,int tabuLength)
{
	bool flag = false;
	for(int i=0;i<tabuLength;i++)
	{
		if(tabuList[i]->swap && aid == tabuList[i]->bid && asource == tabuList[i]->bsource && 
			bid == tabuList[i]->aid && bsource == tabuList[i]->asource ) 
			flag = true;
		if(!tabuList[i]->swap && aid == tabuList[i]->aid && asource == tabuList[i]->bsource && 
			bsource == tabuList[i]->asource ) 
			flag = true;
	}
	return flag;
}

//find best candidate in candidate list
int LocateBestCandidate(Tabu **candidateList, int CLCount, int costA)
{
	int min = 0;
	int bestCostB = 100000;
	int costB;
	for (int i=0;i<CLCount;i++)
	{
		if(candidateList[i]->swap)
		{
			costB = getCost(costA,i,candidateList);
			if(costB <= bestCostB)
			{
				bestCostB = costB;
				min = i;
			}
		}else
		{
			costB = getCostAdd(costA,i,candidateList);
			if(costB <= bestCostB)
			{
				bestCostB = costB;
				min = i;
			}
		}
	}
	return min;
}

int checkAspirationCriteria(Tabu **tabuList,int TLCount,int costA,int cost)
{
	int index = -1;
	for(int i=0;i<TLCount;i++)
	{
		int new_cost;
		if(tabuList[i]->swap)
		{
			new_cost = getCost1(costA,i,tabuList);
		}else
		{
			new_cost = getCostAdd1(costA,i,tabuList);
		}
		if(new_cost<cost)
		{
			index = i;
		}
	}
	return index;
}

void main ()
{
	ifstream input("sample.in",ios::in);
	if(!input)
	{
		cout<< "Error Opening File"<<endl;
		exit(1);
	}
	else
	{
		ofstream output("sample.out",ios::out);
		
		//initialize and input variables		
		input >> blocks >>time_slots >>peers >>M;

		upload_rate = new int[peers];
		for(int i=0;i<peers;i++)
			input >>upload_rate[i];

		download_rate = new int[peers];
		for(int i=0;i<peers;i++)
			input>>download_rate[i];

		seeders = new int*[blocks];
		for(int i=0;i<blocks;i++)
			seeders[i] = new int[peers];

		for(int i=0;i<blocks;i++)
			for(int j=0;j<peers;j++)
				input >> seeders[i][j];

		costs = new int*[peers];
		for(int i=0;i<peers;i++)
			costs[i] = new int[peers];

		for(int i=0;i<peers;i++)
			for(int j=0;j<peers;j++)
				input >> costs[i][j];
		//finish initialize and input variable		
		output << "cost source iteration upload download"<<endl;
		
		clock_t tStart = clock();
		//output <<"cost source iteration totalCost"<<endl;
		int total_cost = 0;
		for (int i =0;i<blocks;i++)
		{
			node ** best = Greedy(i);		//sBest ← s
			int best_cost = getCost(best);

			Tabu **tabuList = new Tabu*[MAX_TABU_SIZE]; //tabuList ← null
			int TLCount =0;

			int iteration = 0;
			while(iteration < MAX_ITERATION)
			{
				//printList(best);
				//output <<endl;
				int CLCount =0;
				Tabu **candidateList = new Tabu*[MAX_CAN_LIST];
				
				for (int i=0;i<peers-1;i++)
				{
					for(int j=i+1;j<peers;j++)
					{
						if(!containsTabuElements(i,j,best[i]->source,best[j]->source, tabuList,TLCount) && best[i]->iteration == best[j]->iteration)
						{ 
							candidateList[CLCount] = new Tabu();
							candidateList[CLCount]->aid=i;
							candidateList[CLCount]->asource=best[i]->source;
							candidateList[CLCount]->bid=j;
							candidateList[CLCount]->bsource = best[j]->source;
							CLCount++;
						}
					}
				}

				for (int i=0;i<peers;i++)
				{
					if(best[i]->leaf)
					{
						for(int j=0;j<peers;j++)
						{
							if(!containsTabuElements(i,0,best[i]->source,j, tabuList,TLCount) && i != j )
							{ 
								candidateList[CLCount] = new Tabu(false);
								candidateList[CLCount]->aid=i;
								candidateList[CLCount]->asource=best[i]->source;
								candidateList[CLCount]->bsource = j;
								CLCount++;
							}
						}
					}
				}
				if(CLCount == 0)
					break;

				int costA = getCost(best);
				int index = LocateBestCandidate(candidateList, CLCount,costA);
				int cost;

				bool isSwap = candidateList[index]->swap;
				if(isSwap)
					cost = getCost(costA,index,candidateList);
				else
					cost = getCostAdd(costA,index,candidateList);
				
				int index1 = checkAspirationCriteria(tabuList,TLCount,costA,cost);

				if(index1 !=-1)
				{
					if(tabuList[index1]->swap)
					{
						int temp = best[tabuList[index1]->aid]->source;
						best[tabuList[index1]->aid]->source = best[tabuList[index1]->bid]->source;
						best[tabuList[index1]->bid]->source = temp;

						best[tabuList[index1]->aid]->cost = costs[best[tabuList[index1]->aid]->source][tabuList[index1]->aid];
						best[tabuList[index1]->bid]->cost = costs[best[tabuList[index1]->bid]->source][tabuList[index1]->bid];

						bool temp1 = best[tabuList[index1]->aid]->leaf;
						best[tabuList[index1]->aid]->leaf = best[tabuList[index1]->bid]->leaf;
						best[tabuList[index1]->bid]->leaf = temp1;
					}
					else
					{
						best[best[tabuList[index1]->aid]->source]->upload--;
						best[tabuList[index1]->aid]->source = tabuList[index1]->bsource;
						best[tabuList[index1]->aid]->iteration = best[tabuList[index1]->bsource]->iteration+1;
						best[tabuList[index1]->bsource]->upload++;
						best[tabuList[index1]->bsource]->leaf=false;
						best[tabuList[index1]->aid]->cost=costs[tabuList[index1]->aid][tabuList[index1]->bsource];
					}
				}
				else
				{
					best_cost = cost;
	
					if(isSwap)
					{
						tabuList[TLCount] = new Tabu();
						tabuList[TLCount]->aid = candidateList[index]->aid;
						tabuList[TLCount]->asource=candidateList[index]->asource;
						tabuList[TLCount]->bid = candidateList[index]->bid;
						tabuList[TLCount]->bsource=candidateList[index]->bsource;
						TLCount++;
				
						int temp = best[candidateList[index]->aid]->source;
						best[candidateList[index]->aid]->source = best[candidateList[index]->bid]->source;
						best[candidateList[index]->bid]->source = temp;

						best[candidateList[index]->aid]->cost = costs[best[candidateList[index]->aid]->source][candidateList[index]->aid];
						best[candidateList[index]->bid]->cost = costs[best[candidateList[index]->bid]->source][candidateList[index]->bid];

						bool temp1 = best[candidateList[index]->aid]->leaf;
						best[candidateList[index]->aid]->leaf = best[candidateList[index]->bid]->leaf;
						best[candidateList[index]->bid]->leaf = temp1;
					}else
					{
						tabuList[TLCount] = new Tabu(false);
						tabuList[TLCount]->aid = candidateList[index]->aid;
						tabuList[TLCount]->asource=candidateList[index]->asource;
						tabuList[TLCount]->bsource=candidateList[index]->bsource;
						TLCount++;
				
						best[best[candidateList[index]->aid]->source]->upload--;
						best[candidateList[index]->aid]->source = candidateList[index]->bsource;
						best[candidateList[index]->aid]->iteration = best[candidateList[index]->bsource]->iteration+1;
						best[candidateList[index]->bsource]->upload++;
						best[candidateList[index]->bsource]->leaf=false;
						best[candidateList[index]->aid]->cost=costs[candidateList[index]->aid][candidateList[index]->bsource];
					}
				}
				
				for(int i=0;i<CLCount;i++)
					delete candidateList[i];
				delete candidateList;

				if(TLCount == MAX_TABU_SIZE)
					TLCount %= MAX_TABU_SIZE;
				iteration++;
			}
			
			printList(best,output);
			total_cost +=best_cost;

			for(int i=0;i<TLCount;i++)
				delete tabuList[i];
			delete tabuList;
			TLCount = 0;
			output<<endl;
			iteration =0;
		}
		cout << "Total Cost: " <<total_cost <<endl;
		cout << "Time taken: " << ((double)(clock() - tStart)/CLOCKS_PER_SEC) <<"sec"<<endl;
		
		output << "Total Cost: " <<total_cost <<endl;
		output << "Time taken: " << ((double)(clock() - tStart)/CLOCKS_PER_SEC) <<"sec"<<endl;
		//clearing memory
		delete upload_rate;
		delete download_rate;
		
		for(int i=0;i<blocks;i++)
			delete seeders[i];
		delete seeders;
		
		for(int i=0;i<peers;i++)
			delete costs[i];
		delete costs;
		//finish clearing memory

		output.close();
	}
	input.close();
}
