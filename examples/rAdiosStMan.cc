//    rAdiosStMan.cc: example code for reading a casa table using AdiosStMan
//
//    (c) University of Western Australia
//    International Centre of Radio Astronomy Research
//    M468, 35 Stirling Hwy
//    Crawley, Perth WA 6009
//    Australia
//
//    This library is free software: you can redistribute it and/or
//    modify it under the terms of the GNU General Public License as published
//    by the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//   
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//   
//    You should have received a copy of the GNU General Public License along
//    with this library. If not, see <http://www.gnu.org/licenses/>.
//
//    Any bugs, questions, concerns and/or suggestions please email to
//    jason.wang@icrar.org



// headers for table creation 
#include <tables/Tables/TableDesc.h>
#include <tables/Tables/SetupNewTab.h>

// headers for scalar column
#include <tables/Tables/ScaColDesc.h>
#include <tables/Tables/ScalarColumn.h>

// headers for array column
#include <casacore/tables/Tables/ArrColDesc.h>
#include <casacore/tables/Tables/ArrayColumn.h>

// headers for casa namespaces
#include <casa/namespace.h>

String filename = "/scratch/tmp/v.casa";

int main (){

	Table casa_table(filename);    

	ROScalarColumn<uInt> index1_col(casa_table, "index1");
	ROScalarColumn<uInt> index2_col(casa_table, "index2");
	ROArrayColumn<float> data_col(casa_table, "data");

	Vector<uInt> index1_vec = index1_col.getColumn();
	Vector<uInt> index2_vec = index2_col.getColumn();
	Array<float> data_arr = data_col.getColumn();

	Vector<float> data_vec = data_arr.reform(IPosition(1,data_arr.nelements()));

	cout << "index1 column: " << endl;
	for (int i=0; i<index1_vec.nelements(); i++){
		cout << index1_vec[i] << "  ";
	}

	cout << endl << endl << "index2 column: " << endl;
	for (int i=0; i<index2_vec.nelements(); i++){
		cout << index2_vec[i] << "  ";
	}

	cout << endl << endl << "data column: " << endl;
	for (int i=0; i<data_arr.nelements(); i++){
		cout << data_vec[i] << "  ";
		if ((i+1) % (data_arr.shape())(0) == 0)	cout << endl;
	}
	return 0;
}


