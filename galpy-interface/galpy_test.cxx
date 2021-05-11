#include <iostream>
#include <fstream>
#include <getopt.h>
#include "galpy_interface.h"
#include "../src/io.hpp"

struct Particle{
    double mass;
    double pos[3];
    double vel[3];
    double acc[3];
    double pot;

    void readAscii(FILE* fp) {
        int rcount=fscanf(fp, "%lf %lf %lf %lf %lf %lf %lf",
                          &mass, &pos[0], &pos[1], &pos[2], &vel[0], &vel[1], &vel[2]);
        if(rcount<7) {
            std::cerr<<"Error: Data reading fails! requiring data number is 7, only obtain "<<rcount<<".\n";
            abort();
        }
    }

    //! print titles of class members using column style
    /*! print titles of class members in one line for column style
      @param[out] _fout: std::ostream output object
      @param[in] _width: print width (defaulted 20)
     */
    static void printColumnTitle(std::ostream & _fout, const int _width=20) {
        _fout<<std::setw(_width)<<"mass"
             <<std::setw(_width)<<"pos.x"
             <<std::setw(_width)<<"pos.y"
             <<std::setw(_width)<<"pos.z"
             <<std::setw(_width)<<"vel.x"
             <<std::setw(_width)<<"vel.y"
             <<std::setw(_width)<<"vel.z"
             <<std::setw(_width)<<"acc.x"
             <<std::setw(_width)<<"acc.y"
             <<std::setw(_width)<<"acc.z"
             <<std::setw(_width)<<"pot";
    }


    //! print data of class members using column style
    /*! print data of class members in one line for column style. Notice no newline is printed at the end
      @param[out] _fout: std::ostream output object
      @param[in] _width: print width (defaulted 20)
     */
    void printColumn(std::ostream & _fout, const int _width=20) const{
        _fout<<std::setw(_width)<<mass
             <<std::setw(_width)<<pos[0]
             <<std::setw(_width)<<pos[1]
             <<std::setw(_width)<<pos[2]
             <<std::setw(_width)<<vel[0]
             <<std::setw(_width)<<vel[1]
             <<std::setw(_width)<<vel[2]
             <<std::setw(_width)<<acc[0]
             <<std::setw(_width)<<acc[1]
             <<std::setw(_width)<<acc[2]
             <<std::setw(_width)<<pot;
    }
};

int main(int argc, char** argv){
    
    double time = 0.0;
    std::string pos_offset_str="0:0:0";
    int arg_label;

    IOParamsGalpy galpy_io;
    opterr = 0;

    // reset optind
    static int long_flag=-1;
    static struct option long_options[] = {
        {"pos-offset", required_argument, &long_flag, 0},
        {0,0,0,0}
    };
    
    int opt_used = 0;
    optind=0;
    int option_index;
    bool help_flag=false;
    bool unit_astro_flag = false;

    while ((arg_label = getopt_long(argc, argv, "-t:uh", long_options, &option_index)) != -1)
        switch (arg_label) {
        case 0:
            switch (long_flag) {
            case 0:
                pos_offset_str = optarg;
                std::cout<<"Position offset: "<<pos_offset_str<<std::endl;
                opt_used+=2;
                break;
            default:
                break;
            }
            break;
        case 't':
            time = atof(optarg);
            std::cout<<"Time: "<<time<<std::endl;
            opt_used +=2;
            break;
        case 'u':
            unit_astro_flag = true;
            std::cout<<"Use the astronomical unit set (Myr, pc, Msun)\n";
            opt_used ++;
            break;
        case 'h':
            std::cout<<"The tool to calculate acceleration and potential for a given particle list \n"
                     <<"Usage: petar.galpy [options] [particle data file]\n"
                     <<"       data file format: first line: number of particles\n"
                     <<"                         following lines: mass, pos(3), vel(3)\n"
                     <<"Options:\n"
                     <<"    -u    : input data use astronomical unit set (Myr, pc, Msun) and set unit scaling factor for Galpy automatically\n"
                     <<"    -t [F]: time (0.0)\n"
                     <<"    --pos-offset: [S]: 3D position offset in unit of the input particle data (IN) to obtain the particle position in the potential field, x, y, z are separated by ':', (0:0:0)\n"
                     <<"    -h    : help\n";
            help_flag=true;
            break;
        case '?':
            opt_used +=2;
            break;
        default:
            break;
        }
    
    galpy_io.print_flag = true;
    galpy_io.read(argc,argv);
    
    if (help_flag) return 0;

    GalpyManager galpy_manager;
    if (unit_astro_flag) galpy_io.setStdUnit();
    galpy_manager.initial(galpy_io,true);

    // position offset
    double pos_offset[3]; 
    std::size_t istart = 0;
    std::size_t inext = pos_offset_str.find_first_of(":");
    if (inext==std::string::npos) {
        std::cerr<<"Error: position offset delimiter ':' is not found in input string "<<pos_offset_str<<std::endl;
        abort();
    }
    pos_offset[0] = std::stod(pos_offset_str.substr(0,inext));
    istart = inext + 1;
    inext = pos_offset_str.find_first_of(":",istart);
    if (inext==std::string::npos) {
        std::cerr<<"Error: 2nd position offset delimiter ':' is not found in input string "<<pos_offset_str<<std::endl;
        abort();
    }
    pos_offset[1] = std::stod(pos_offset_str.substr(istart,inext-istart));
    pos_offset[2] = std::stod(pos_offset_str.substr(inext+1));
    std::cout<<"Central position offset: "<<pos_offset[0]<<" "<<pos_offset[1]<<" "<<pos_offset[2]<<std::endl;

    // argc is 1 no input is given
    opt_used++;
    std::string filename;
    if (opt_used<argc) {
        // read initial mass list
        filename = argv[opt_used++];
    }

    FILE* fp;
    if( (fp = fopen(filename.c_str(),"r")) == NULL) {
        fprintf(stderr,"Error: Cannot open file %s.\n", filename.c_str());
        abort();
    }
    int n = 0;
    int rcount=fscanf(fp, "%d", &n);
    if(rcount<1) {
        std::cerr<<"Error: Data reading fails! requiring data number is 1, only obtain "<<rcount<<".\n";
        abort();
    }

    assert(n>0);

    Particle particles[n];

    Particle::printColumnTitle(std::cout);
    std::cout<<std::endl;

    for (int i=0; i<n; i++) {
        particles[i].readAscii(fp);
        double pos[3] = {particles[i].pos[0] + pos_offset[0],
                         particles[i].pos[1] + pos_offset[1],
                         particles[i].pos[2] + pos_offset[2]};
        galpy_manager.calcAccPot(particles[i].acc, particles[i].pot, time, pos, &particles[i].pos[0]);

        particles[i].printColumn(std::cout);
        std::cout<<std::endl;
    }
    
    return 0;
}
