/*
 * EvaluationFunction.cpp
 *
 *  Created on: 8 may. 2018
 *      Author: iaraya
 */

#include "EvaluationFunction.h"
#include <string>
#include <functional>

namespace imrt {

EvaluationFunction::EvaluationFunction(vector<Volume>& volumes) : prev_F(0.0), F(0.0),
	nb_organs(volumes.size()), nb_voxels(volumes.size()), voxel_dose(volumes.size(), vector<double>(100)) {

	for(int i=0; i<nb_organs; i++){
		nb_voxels[i]=volumes[i].getNbVoxels();
		Z.insert(Z.end(), vector<double>(nb_voxels[i]));
		D.insert(D.end(), vector<double>(nb_voxels[i]));
	}
}

EvaluationFunction::~EvaluationFunction() { }


void EvaluationFunction::generate_Z(const Plan& p){

	const list<Station*>& stations=p.get_stations();

	for(int o=0; o<nb_organs; o++)
	 	 std::fill(Z[o].begin(), Z[o].end(), 0.0);

	for(auto station : stations){
		//considering 2*Xmid, Xext
		//we update the dose distribution matrices Z with the dose delivered by the station
		for(int o=0; o<nb_organs; o++){
			 const Matrix&  D = station->getDepositionMatrix(o);
			 for(int k=0; k<nb_voxels[o]; k++){
			   double dose=0.0;
			   for(int b=0; b<station->getNbBeamlets(); b++)
					 dose +=  D(k,b)*station->getIntensity( b );
			   Z[o][k] += dose;
			 }
		}
	}
}


double EvaluationFunction::eval(const Plan& p, vector<double>& w, vector<double>& Zmin, vector<double>& Zmax){
	voxels.clear();
  voxels.clear();

	generate_Z(p);

	F=0.0;

	for(int o=0; o<nb_organs; o++)
		for(int k=0; k<nb_voxels[o]; k++){
			double pen=0.0;
			if(Z[o][k] < Zmin[o] ){
				 pen = w[o] * ( pow(Zmin[o]-Z[o][k], 2) );
			 }

			if(Z[o][k] > Zmax[o] ){
				 pen = w[o] * ( pow(Z[o][k]-Zmax[o], 2) );
			 }
			F+= pen;

			update_sorted_voxels(w, Zmin, Zmax, o, k, false);
		}


	return F;
}

void EvaluationFunction::update_sorted_voxels(vector<double>& w,
	vector<double>& Zmin, vector<double>& Zmax, int o, int k, bool erase){
		if(erase) voxels.erase(make_pair(abs(D[o][k]),make_pair(o,k)));
		if(Zmin[o]>0){
			 if(Z[o][k] < Zmin[o]) D[o][k]=w[o]*(Z[o][k]-Zmin[o]);
			 else D[o][k]=0.0;
		}else{
			if(Z[o][k] > Zmax[o]) D[o][k]=w[o]*(Z[o][k]-Zmax[o]);
			else D[o][k]=0.0;
		}
		voxels.insert(make_pair(abs(D[o][k]),make_pair(o,k)));
}

double EvaluationFunction::incremental_eval(Station& station, vector<double>& w,
	vector<double>& Zmin, vector<double>& Zmax, list< pair< int, double > >& diff){

  prev_F=F; Z_diff.clear();
  double delta_F=0.0;

  //for each voxel we compute the change produced by the modified beamlets
  //while at the same time we compute the variation in the function F produced by all these changes

  for(int o=0; o<nb_organs; o++){
		const Matrix&  D = station.getDepositionMatrix(o);
		for(int k=0; k<nb_voxels[o]; k++){

		//we compute the change in the delivered dose in voxel k of the organ o
		double delta=0.0;

		//cout << station.changed_lets.size() << endl;
		for (auto let:diff){
		    int b=let.first;
			if(D(k,b)==0.0) continue;
				delta+= D(k,b)*let.second;
		}


		if(delta==0.0) continue; //no change in the voxel


		double pen=0.0;
		//with the change in the dose of a voxel we can incrementally modify the value of F
		if(Z[o][k] < Zmin[o] && Z[o][k] + delta < Zmin[o]) //update the penalty
			pen += w[o]*delta*(delta+2*(Z[o][k]-Zmin[o]));
		else if(Z[o][k] < Zmin[o]) //the penalty disappears
			pen -=  w[o] * ( pow(Zmin[o]-Z[o][k], 2) );
		else if(Z[o][k] + delta < Zmin[o]) //the penalty appears
			pen +=  w[o] * ( pow(Zmin[o]-(Z[o][k]+delta), 2) );

		if(Z[o][k] > Zmax[o] && Z[o][k] + delta > Zmax[o]) //update the penalty
			pen += w[o]*delta*(delta+2*(-Zmax[o] + Z[o][k]));
		else if(Z[o][k] > Zmax[o]) //the penalty disappears
			pen -=  w[o] * ( pow(Z[o][k]-Zmax[o], 2) );
		else if(Z[o][k] + delta > Zmax[o]) //the penalty appears
			pen +=  w[o] * ( pow(Z[o][k]+delta - Zmax[o], 2) );

		delta_F += pen;
		Z[o][k]+=delta;

		update_sorted_voxels(w, Zmin, Zmax, o, k);

		//we save the last changes (see undo_last_eval)
		Z_diff.push_back(make_pair(make_pair(o,k),delta));
	}
  }


  F+=delta_F;
  return F;

  //return eval(p, false);

}

void EvaluationFunction::undo_last_eval(vector<double>& w,
	vector<double>& Zmin, vector<double>& Zmax){

	for(auto z:Z_diff){
		int o=z.first.first;
		int k=z.first.second;
		Z[o][k]-=z.second;

		update_sorted_voxels(w, Zmin, Zmax, o, k);
	}
	F=prev_F;
	prev_F=F; Z_diff.clear();



}


	set < pair< pair<double,bool>, pair<Station*, int> >,
	std::greater < pair< pair<double,bool>, pair<Station*, int> > > >
EvaluationFunction::best_beamlets(Plan& p, int n, int nv){

 set < pair< pair<double,bool>, pair<Station*, int> >,
	std::greater < pair< pair<double,bool>, pair<Station*, int> > > >  bestb;

	double max_ev=0.0;
	for(auto s:p.get_stations()){
		for(int b=0; b<s->getNbBeamlets(); b++){
			double ev=0; int i=0;
			for(auto voxel:voxels){
				int o=voxel.second.first;
				int k=voxel.second.second;
				const Matrix&  Dep = s->getDepositionMatrix(o);
				ev += D[o][k] * Dep(k,b);
				i++; if(i==nv) break;
			}

			if( bestb.size() < n || abs(ev) > abs(bestb.rbegin()->first.first)){
				bestb.insert(make_pair( make_pair(abs(ev), (ev>0)), make_pair(s,b)));
				if(bestb.size() > n){
					auto it = bestb.end();  --it;
					bestb.erase(it);
				}
			}
		}
	}

	return bestb;
}

void EvaluationFunction::generate_voxel_dose_functions (){
	for(int o=0; o<nb_organs; o++){
		std::fill(voxel_dose[o].begin(), voxel_dose[o].end(), 0.0);
		for(int k=0; k<nb_voxels[o]; k++){
			if(Z[o][k]<100)
				voxel_dose[o][(int) Z[o][k]]+=1;
		}
	}


	for(int o=0; o<nb_organs; o++){
		ofstream myfile;
		myfile.open ("plotter/organ"+std::to_string(o)+".txt");

		double cum=0.0;
		for(int k=99; k>=0; k--){
			cum+= voxel_dose[o][k];
			myfile << k+1 << "," << cum << endl;
		}

		myfile.close();
	}
}




} /* namespace imrt */
