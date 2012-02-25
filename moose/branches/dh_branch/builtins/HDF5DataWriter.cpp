// HDF5DataWriter.cpp --- 
// 
// Filename: HDF5DataWriter.cpp
// Description: 
// Author: Subhasis Ray
// Maintainer: 
// Created: Sat Feb 25 16:03:59 2012 (+0530)
// Version: 
// Last-Updated: Sat Feb 25 20:10:16 2012 (+0530)
//           By: Subhasis Ray
//     Update #: 232
// URL: 
// Keywords: 
// Compatibility: 
// 
// 

// Commentary: 
// 
// 
// 
// 

// Change log:
// 
// 2012-02-25 16:04:02 (+0530) Subha - started initial implementation
// 

// Code:

#ifdef USE_HDF5
#include "HDF5DataWriter.h"

HDF5DataWriter::CHUNK_SIZE = 1024; // default chunk size
const Cinfo * HDF5DataWriter::initCinfo()
{
    static Finfo * processShared[] = {
        new DestFinfo(
                "procss",
                "Handle process calls. Write data to file and clear all Table objects"
                " associated with this.",
                new ProcOpFunc<HDF5DataWriter>(&HDF5DataWriter::process)),
        new DestFinfo(
                "reinit",
                "Reinitialize the object",
                new ProcOpFunc<HDF5DataWriter>(&HDF5DataWriter::reinit)),
    };
    static Finfo * finfos[] = {
        new DestFinfo<HDF5DataWriter>(
                "flush",
                "Flush all data from associated Table objects to file.",
                OpFunc0(&HDF5DataWriter::flush)),
        new SharedFinfo(
                "proc",
                "This is a shared message to receive Process messages from the scheduler objects."
                "The first entry in the shared msg is a MsgDest for the Process"
                " operation. It has a single argument, ProcInfo, which holds lots of"
                " information about current time, thread, dt and so on. The second entry"
                " is a MsgDest for the Reinit operation. It also uses ProcInfo.",
                processShared, sizeof( processShared ) / sizeof( Finfo* )),
    };

    static string doc[] = {
        "Name", "HDF5DataWriter",
        "Author", "Subhasis Ray",
        "Description", "HDF5 file writer for saving data tables. It saves the tables added to"
        " it via addObject function into an HDF5 file. At every process call it"
        " writes the contents of the tables to the file and clears the table"
        " vectors. You can explicitly save the data via the flush function."
    };

    static Cinfo cinfo(
            "HDF5DataWriter",
            HDF5WriterBase::initCinfo(),
            finfos,
            sizeof(finfos)/sizeof(Finfo*),
            new Dinfo<HDF5DataWriter>());
    return &cinfo;
}

static const Cinfo * hdf5dataWriterCinfo = HDF5DataWriter::initCinfo();

HDF5DataWriter::HDF5DataWriter()
{
    ;
}

HDF5DataWriter::~HDF5DataWriter()
{
    ;
}

/**
   Write data to datasets in HDF5 file. Clear all data in the table
   objects associated with this object. */
void HDF5DataWriter::process(const Eref & e, ProcPtr p)
{
    herr_t status;
    for (map <ObjId, hid_t > it = object_node_map_.begin(); it != object_node_map_.end(); ++it){
        vector <double> vec = Field < vector < double > >::get(it->first, "vec");
        hid_t filespace = H5Dget_space(it->second);
        hssize_t size = H5Sget_simple_extent_npints(filespace) + vec.size();
        status = H5Dset_extent(it->second, &size);
        filespace = H5Dget_space(it->second);
        hid_t memspace = H5Screate_simple(vec.size());
        H5Sselect_hyperslab(filespace, size - vec.size(), vec.size());
        H5Dwrite(it->second, H5T_NATIVE_DOUBLE, memspace, filespace, H5P_DEFAULT, &vec[0]);
    }
}

void HDF5DataWriter::reinit(const Eref & e, ProcPtr p)
{
    for (map<ObjId, hid_t>::iterator it = object_node_map_.begin(); it != object_node_map_.end(); ++it){
        hid_t dataset_id = get_dataset(it->first);
        if (dataset_id < 0){
            cerr << "Warning: could not get dataset for " << it->first.path() << endl;            
        }
        object_node_map_[it->first] = dataset_id;
    }
}

/**
   Traverse the path of an object in HDF5 file, checking existence of
   groups in the path and creating them if required.  */
hid_t HDF5DataWriter::get_dataset(ObjId oid)
{
    // Create the groups corresponding to this path We are not
    // taking care of Table object containing Table
    // objects. That's an unusual possibility.
    vector<string> path_tokens;
    tokenize(oid.path(), "/", path_tokens);
    hid_t prev_id = filehandle_;
    for ( unsigned int ii = 0; ii < path_tokens.size(); ++ii ){
        // First try to open existing group
        hid_t id = H5Gopen(prev_id, path_tokens[ii], H5P_DEFAULT);            
        if (id < 0){
            // If that fails, try to create a group
            id = H5Fcreate(prev_id, path_tokens[ii], H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            if (id < 0){
                // Failed to craete a group also, print the
                // offending path (for debugging; the error is
                // perhaps at the level of hdf5 or file system).
                cerr << "Error: failed to open/create group: ";
                for (int jj = 0; jj <= ii; ++jj){
                    cerr << "/" << path_tokens[jj];
                }
                cerr << endl;
                prev_id = -1;
                break;
            } else if (prev_id != filehandle_){
                // Successfully created new group, close the old group
                status = H5Gclose(prev_id);
                prev_id = id;
            }
        } else if (prev_id != filehandle_){
            // Successfully opened new group, close the old group
            status = H5Gclose(prev_id);
            prev_id = id;
        }
    }
    if (prev_id < 0){
        object_node_map_[it->first] = prev_id;
        continue;
    }
    // first try to open, then try to create dataset    
    hid_t dataset_id = H5Dopen(prev_id, path_tokens[0], H5P_DEFAULT);
    if (dataset_id < 0){
        dataset_id = create_dataset();
    } else {
        hid_t dataspace = H5Dget_space(dataset_id);
        hssize_t size = H5Sget_simple_extent_npints(dataspace);
    }
    return dataset_id;
}

/**
   Create a new 1D dataset. Make it extensible.
*/
hid_t HDF5DataWriter::create_dataset(hid_t parent_id, string name, ObjId oid)
{
    hsize_t dims[1] = {0};
    hsize_t maxdims[] = {H5S_UNLIMITED};
    hsize_t chunk_dims[] = {CHUNK_SIZE}; // 1 K
    hid_t chunk_params = H5Pcreate(H5P_DATASET_CREATE);
    status = H5Pset_chunk(chunk_params, 1, chunk_dims);
    hid_t dataspace = H5Screate_simple(1, dims, maxdims);            
    hid_t dataset_id = H5Dcreate(parent_id, name, H5T_NATIVE_DOUBLE, dataspace, H5P_DEFAULT, chunk_params, H5P_DEFAULT);
    return dataset_id;
}

    
#endif // USE_HDF5
// 
// HDF5DataWriter.cpp ends here
