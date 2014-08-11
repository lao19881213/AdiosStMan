//    AdiosStMan.cc: Storage Manager for general CASA tables using ADIOS
//    as the underlying IO & storage library
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

#include "AdiosStMan.h"
#include "AdiosStManColumn.h"


namespace casa { 

	AdiosStMan::AdiosStMan ()
		:DataManager(),
		itsAdiosFile(0),
		isMpiInitInternal(true)
	{
		MPI_Init(0,0);
		MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
		MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
		cout << "AdiosStMan::AdiosStMan() | rank = " << mpiRank << endl;
	}

	AdiosStMan::AdiosStMan (int rank, int size)
		:DataManager(),
		itsAdiosFile(0),
		isMpiInitInternal(false)
	{
		mpiRank = rank;
		mpiSize = size;
		cout << "AdiosStMan::AdiosStMan(int,int) | rank = " << mpiRank << endl;
	}

	AdiosStMan::AdiosStMan (const AdiosStMan& that)
		:DataManager(),
		itsAdiosFile(0),
//		mpiRank(that.mpiRank),
//		mpiSize(that.mpiSize),
		isMpiInitInternal(false)
	{
		cout << "AdiosStMan::AdiosStMan(const AdiosStMan&) | rank = " << mpiRank << endl;
	}

	AdiosStMan::~AdiosStMan ()
	{
		if(itsAdiosFile){
			adios_close(itsAdiosFile);
			adios_finalize(mpiRank);
		}
		if(isMpiInitInternal){
			MPI_Finalize();
		}
	}


	DataManager* AdiosStMan::clone() const
	{
		cout << "AdiosStMan::clone | rank = " << mpiRank << endl;
		return new AdiosStMan (*this);
	}

	String AdiosStMan::dataManagerType() const
	{
		return "AdiosStMan";
	}

	int64_t AdiosStMan::getAdiosFile(){ 
		return itsAdiosFile;
	}

	void AdiosStMan::create (uInt aNrRows)
	{

		itsNrRows = aNrRows;

		adios_init_noxml(MPI_COMM_WORLD);

		adios_declare_group(&itsAdiosGroup, "casatable", "", adios_flag_no);
		adios_select_method(itsAdiosGroup, "POSIX", "", "");

		itsAdiosGroupsize = 0;

		// loop for columns
		for (int i=0; i<ncolumn(); i++){
			string columnName = itsColumnPtrBlk[i]->columnName();
			itsColumnPtrBlk[i]->initAdiosWriteIDs(itsNrRows);

			// if scalar column
			if (itsColumnPtrBlk[i]->getShapeColumn().nelements() == 0){   
				// loop for rows
				for (int j=0; j<itsNrRows; j++){
					stringstream NrRows, RowID;
					NrRows << itsNrRows;
					RowID << j;
					int64_t writeID = adios_define_var(itsAdiosGroup, columnName.c_str(), "", itsColumnPtrBlk[i]->getAdiosDataType(), "1", NrRows.str().c_str(), RowID.str().c_str() );
					itsColumnPtrBlk[i]->putAdiosWriteIDs(j, writeID);
				}
				itsAdiosGroupsize = itsAdiosGroupsize + itsNrRows * itsColumnPtrBlk[i]->getDataTypeSize();
			}


			// if array column
			else{
				string columnShape = itsColumnPtrBlk[i]->getShapeColumn().toString();
				columnShape = columnShape.substr(1, columnShape.length()-2);

				// loop for rows
				for (int j=0; j<itsNrRows; j++){
					stringstream NrRows, RowID;
					NrRows << itsNrRows;
					RowID << j;
					string dimensions = "1," + columnShape;
					string global_dimensions = NrRows.str() + "," + columnShape;
					string local_offsets = RowID.str(); 
					for (int k=0; k<itsColumnPtrBlk[i]->getShapeColumn().nelements(); k++){
						local_offsets += ",0";
					}
					int64_t writeID = adios_define_var(itsAdiosGroup, columnName.c_str(), "", itsColumnPtrBlk[i]->getAdiosDataType(), dimensions.c_str(), global_dimensions.c_str(), local_offsets.c_str());
					itsColumnPtrBlk[i]->putAdiosWriteIDs(j, writeID);
				}
				itsAdiosGroupsize = itsAdiosGroupsize + itsNrRows * itsColumnPtrBlk[i]->getDataTypeSize() * itsColumnPtrBlk[i]->getShapeColumn().product();
			}
		}

		itsAdiosBufsize = itsAdiosGroupsize * 1.05 / 1000000;
		if(itsAdiosBufsize < 100) itsAdiosBufsize = 100;
		adios_allocate_buffer(ADIOS_BUFFER_ALLOC_NOW, itsAdiosBufsize);

		adios_open(&itsAdiosFile, "casatable", fileName().c_str(), "w", MPI_COMM_WORLD);

		adios_group_size(itsAdiosFile, itsAdiosGroupsize, &itsAdiosTotalsize);

	}

	void AdiosStMan::deleteManager()
	{

	}

	DataManagerColumn* AdiosStMan::makeScalarColumn (const String& name,
			int aDataType,
			const String& dataTypeId)
	{
		cout << "AdiosStMan::makeScalarColumn | rank = " << mpiRank << endl;
		makeDirArrColumn(name, aDataType, dataTypeId);
	}

	DataManagerColumn* AdiosStMan::makeDirArrColumn (const String&,
			int aDataType,
			const String&)
	{
		cout << "AdiosStMan::makeDirArrColumn | rank = " << mpiRank << endl;
		if (ncolumn() >= itsColumnPtrBlk.nelements()) {
			itsColumnPtrBlk.resize (itsColumnPtrBlk.nelements() + 32);
		}
		AdiosStManColumn* aColumn = new AdiosStManColumn (this, aDataType, ncolumn());
		itsColumnPtrBlk[ncolumn()] = aColumn;
		return aColumn;
	}

	DataManagerColumn* AdiosStMan::makeIndArrColumn (const String& name,
			int aDataType,
			const String& dataTypeId)
	{
		cout << "AdiosStMan error: Indirect arrays are currently not supported in AdiosStMan!" << endl;
	}

	void AdiosStMan::open (uInt aRowNr, AipsIO& ios)
	{
		cout << "AdiosStMan::open" << endl;

		adios_init_noxml(MPI_COMM_WORLD);
		adios_open(&itsAdiosFile, "casatable", fileName().c_str(), "w", MPI_COMM_WORLD);
	}

	void AdiosStMan::resync (uInt aNrRows)
	{

	}

	Bool AdiosStMan::flush (AipsIO& ios, Bool doFsync)
	{

	}

} // end of namespace casa

