#include "helper.h"

int parseArgs(params* config, int argc, char* argv[])
{
    // check the number of arguments
    if (argc < 16 || argc > 18)
    {
        std::cerr << "SiMpLE usage:"                                << std::endl <<
                     "./simple "                                    << std::endl <<
                     "--path                    \"path_to_scans\" " << std::endl <<
                     "--sigma                   \"sigma_value\" "   << std::endl <<
                     "--rMap                    \"radius [m]\" "    << std::endl <<
                     "--rNew                    \"radius [m]\" "    << std::endl <<
                     "--convergenceTolerance    \"tolerance\" "     << std::endl <<
                     "--maxSensorRange          \"radius [m]\" "    << std::endl <<
                     "--minSensorRange          \"radius [m]\" "    << std::endl <<
                     "--outputFileName          \"fileName\" "      << std::endl <<
                     "--verbose (optional)"                         << std::endl;
        return 1;
    }
    
    // parse the arguments
    for (unsigned int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i],"--path") == 0)
        {
            config->path = argv[i+1];        
        }
        if (strcmp(argv[i],"--sigma") == 0)
        {
            config->sigma = std::stod(argv[i+1]);        
        }
        if (strcmp(argv[i],"--rMap") == 0)
        {
            config->rMap = std::stod(argv[i+1]);        
        }
        if (strcmp(argv[i],"--rNew") == 0)
        {
            config->rNew = std::stod(argv[i+1]);        
        }
        if (strcmp(argv[i],"--convergenceTolerance") == 0)
        {
            config->convergenceTol = std::stod(argv[i+1]);        
        }
        if (strcmp(argv[i],"--maxSensorRange") == 0)
        {
            config->maxSensorRange = std::stod(argv[i+1]);        
        }
        if (strcmp(argv[i],"--minSensorRange") == 0)
        {
            config->minSensorRange = std::stod(argv[i+1]);        
        }
        if (strcmp(argv[i],"--outputFileName") == 0)
        {
            config->outputFileName = argv[i+1];        
        }
        if (strcmp(argv[i],"--verbose") == 0)
        {
            config->verbose = true;        
        }
    }

    // print out the configuration parameters
    if (config->verbose)
    {
        std::cout << "--------------------" << std::endl;
        std::cout << "PARAMETERS ---------" << std::endl;
        std::cout << "--------------------" << std::endl;
        std::cout << "scansFolderPath        = " << "\"" <<  config->path << "\""   << std::endl;
        std::cout << "sigma                  = " << config->sigma                   << std::endl;
        std::cout << "rMap                   = " << config->rMap                    << std::endl;
        std::cout << "rNew                   = " << config->rNew                    << std::endl;
        std::cout << "convergenceTolerance   = " << config->convergenceTol          << std::endl;
        std::cout << "maxSensorRange         = " << config->maxSensorRange          << std::endl;
        std::cout << "minSensorRange         = " << config->minSensorRange          << std::endl;
        std::cout << "outputFileName         = " << config->outputFileName          << std::endl;
        std::cout << "--------------------" << std::endl;
    }

    return 0;
}

Eigen::Matrix4d homogeneous(double roll, double pitch, double yaw, 
                            double x, double y, double z)
{
    Eigen::Matrix4d T;
    T.setZero();

    T(0,0) = cos(yaw)*cos(pitch);
    T(0,1) = cos(yaw)*sin(pitch)*sin(roll)-sin(yaw)*cos(roll);
    T(0,2) = cos(yaw)*sin(pitch)*cos(roll)+sin(yaw)*sin(roll);
    T(0,3) = x;
    T(1,0) = sin(yaw)*cos(pitch);
    T(1,1) = sin(yaw)*sin(pitch)*sin(roll)+cos(yaw)*cos(roll);
    T(1,2) = sin(yaw)*sin(pitch)*cos(roll)-cos(yaw)*sin(roll);
    T(1,3) = y;
    T(2,0) = -sin(pitch);
    T(2,1) = cos(pitch)*sin(roll);
    T(2,2) = cos(pitch)*cos(roll);
    T(2,3) = z;
    T(3,3) = 1;

    return T;
}

std::vector<double> hom2rpyxyz(Eigen::Matrix4d T)
{
    double ROLL = atan2(T(2,1),T(2,2));
    double PITCH = asin(-T(2,0));
    double YAW = atan2(T(1,0),T(0,0));
    double X = T(0,3);
    double Y = T(1,3);
    double Z = T(2,3);
    std::vector<double> result = {ROLL,PITCH,YAW,X,Y,Z};
    return result;
}

void convertToPointCloud3D(PointCloud<double>& pc, scan &pointcloudRead)
{
    size_t pcLength = pointcloudRead.x.size();
    pc.pts.resize(pcLength);

    for (size_t i = 0; i < pcLength; i++)
    {
        pc.pts[i].x = pointcloudRead.x[i];
        pc.pts[i].y = pointcloudRead.y[i];
        pc.pts[i].z = pointcloudRead.z[i];
        
    } 
}

bool compareStrings(std::string a, std::string b)
{
    std::string delimiterStart = "/";
    // std::string delimiterStart = "subset/";
    std::string delimiterEnd = ".bin";
    
    std::string aNum = a.substr(a.find_last_of(delimiterStart)+delimiterStart.size(),a.size());
    std::string bNum = b.substr(b.find_last_of(delimiterStart)+delimiterStart.size(),b.size());
    aNum = aNum.substr(0, aNum.find(delimiterEnd));
    bNum = bNum.substr(0, bNum.find(delimiterEnd));

    return stoi(aNum) < stoi(bNum);
}

scan subsample(double subsampleRadius, std::set<int> allPoints, scan pts)
{
    scan scanSubsampled;    
    PointCloud<double> scanToSubsample;
    convertToPointCloud3D(scanToSubsample,pts);

    // create a Kd tree
    my_kd_tree_t *scanKdTree = new my_kd_tree_t(3,scanToSubsample,{10});
    
    // subsample radially
    for (unsigned int i : allPoints)
    {
        std::vector<nanoflann::ResultItem<uint32_t, double>> ret_matches;
        const double query_pt[3] = {pts.x[i],pts.y[i],pts.z[i]};
        const size_t nMatches = scanKdTree->radiusSearch(&query_pt[0], subsampleRadius, ret_matches);
        for (unsigned int j = 0; j < nMatches; j++)
        {
            if (i != ret_matches[j].first)
            {
                allPoints.erase(ret_matches[j].first);
            }
        }
        scanSubsampled.x.push_back(pts.x[i]);
        scanSubsampled.y.push_back(pts.y[i]);
        scanSubsampled.z.push_back(pts.z[i]);
    }
    delete scanKdTree; // free memory
    return scanSubsampled;
}

void writeResults(params* config, std::vector<std::vector<double> > poseEstimates, std::string outputFileName, double avgTimePerScan)
{
    // write the pose estimates to file
    std::ofstream outputResultsFile(outputFileName);
    for (unsigned int i = 0; i < poseEstimates.size(); i++)
    {
        Eigen::Matrix4d res = homogeneous(poseEstimates[i][0],
                                          poseEstimates[i][1],
                                          poseEstimates[i][2],
                                          poseEstimates[i][3],
                                          poseEstimates[i][4],
                                          poseEstimates[i][5]);
        outputResultsFile << res(0,0) << " " 
                          << res(0,1) << " "
                          << res(0,2) << " "
                          << res(0,3) << " "
                          << res(1,0) << " "
                          << res(1,1) << " "
                          << res(1,2) << " "
                          << res(1,3) << " "
                          << res(2,0) << " "
                          << res(2,1) << " "
                          << res(2,2) << " "
                          << res(2,3) << std::endl;
    }
    outputResultsFile.close();

    // get the time now
    auto end = std::chrono::system_clock::now();
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);

    // write the config to file
    std::string outputConfigFileName = outputFileName+"_config";
    std::ofstream outputConfigFile(outputConfigFileName);
    outputConfigFile << "computation finised at : " << std::ctime(&end_time)
                     << "avg time per scan [ms] : " << avgTimePerScan << std::endl
                     << std::endl
                     << "scansFolderPath        = " << "\"" <<  config->path << "\"" << std::endl
                     << "sigma [m]              = " << config->sigma                 << std::endl
                     << "rMap [m]               = " << config->rMap                  << std::endl
                     << "rNew [m]               = " << config->rNew                  << std::endl
                     << "convergenceTolerance   = " << config->convergenceTol        << std::endl
                     << "maxSensorRange [m]     = " << config->maxSensorRange        << std::endl
                     << "minSensorRange [m]     = " << config->minSensorRange        << std::endl
                     << "outputFileName         = " << config->outputFileName        << std::endl
                     << "outputConfigFileName   = " << outputConfigFileName          << std::endl;
    outputConfigFile.close();
}

// code from https://stackoverflow.com/questions/14539867/how-to-display-a-progress-indicator-in-pure-c-c-cout-printf
void printProgress(double percentage) {
    int progressBarWidth = 60;
    char progressBarString[] = "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||";
    int val = (int) (percentage * 100);
    int lpad = (int) (percentage * progressBarWidth);
    int rpad = progressBarWidth - lpad;
    printf("\r%3d%% [%.*s%*s]", val, lpad, progressBarString, rpad, "");
    fflush(stdout);
}