#pragma once
#include<particle_simulator.hpp>
#include<iomanip>
#include<iostream>
//#include<fstream>
#include<map>

/*
template<class Tdinfo, class Tsystem, class Ttree>
class Profile{

private:
    Tdinfo * dinfo_;
    Tsystem * system_;
    Ttree * tree_; 
    PS::F64 n_op_ep_ep_;
    PS::F64 n_op_ep_sp_;
    PS::F64 flops_per_core_;
public:

    static void dumpTimeProfile(const PS::TimeProfile & tp, const PS::TimeProfile & tp_max, const PS::S32 rank_max[], std::ostream & fout){
        PS::S32 id = 0;
        fout<<"collect_sample_particle= "<<tp.collect_sample_particle<<", max= "<<tp_max.collect_sample_particle<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"decompose_domain= "<<tp.decompose_domain<<", max= "<<tp_max.decompose_domain<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"exchange_particle= "<<tp.exchange_particle<<", max= "<<tp_max.exchange_particle<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"set_particle_local_tree= "<<tp.set_particle_local_tree<<", max= "<<tp_max.set_particle_local_tree<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"set_root_cell= "<<tp.set_root_cell<<", max= "<<tp_max.set_root_cell<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"make_local_tree= "<<tp.make_local_tree<<", max= "<<tp_max.make_local_tree<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"    morton_sort_local_tree= "<<tp.morton_sort_local_tree<<", max= "<<tp_max.morton_sort_local_tree<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"    link_cell_local_tree= "<<tp.link_cell_local_tree<<", max= "<<tp_max.link_cell_local_tree<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"calc_moment_local_tree= "<<tp.calc_moment_local_tree<<", max= "<<tp_max.calc_moment_local_tree<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"make_LET_1st= "<<tp.make_LET_1st<<", max= "<<tp_max.make_LET_1st<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"exchange_LET_1st= "<<tp.exchange_LET_1st<<", max= "<<tp_max.exchange_LET_1st<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"make_LET_2nd= "<<tp.make_LET_2nd<<", max= "<<tp_max.make_LET_2nd<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"exchange_LET_2nd= "<<tp.exchange_LET_2nd<<", max= "<<tp_max.exchange_LET_2nd<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"set_particle_global_tree= "<<tp.set_particle_global_tree<<", max= "<<tp_max.set_particle_global_tree<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"    morton_sort_global_tree= "<<tp.morton_sort_global_tree<<", max= "<<tp_max.morton_sort_global_tree<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"    link_cell_global_tree= "<<tp.link_cell_global_tree<<", max= "<<tp_max.link_cell_global_tree<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"make_global_tree= "<<tp.make_global_tree<<", max= "<<tp_max.make_global_tree<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"calc_moment_global_tree= "<<tp.calc_moment_global_tree<<", max= "<<tp_max.calc_moment_global_tree<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"calc_force = "<<tp.calc_force<<", max= "<<tp_max.calc_force<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<std::endl;
    } 

    static void dumpTimeProfile0(const PS::TimeProfile & tp, const PS::TimeProfile & tp_max, const PS::S32 rank_max[], std::ostream & fout){
        PS::S32 id = 0;
        fout<<"collect_sample_particle= "<<tp.collect_sample_particle<<", max= "<<tp_max.collect_sample_particle<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"decompose_domain= "<<tp.decompose_domain<<", max= "<<tp_max.decompose_domain<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<std::endl;
    } 

    static void dumpTimeProfile1(const PS::TimeProfile & tp, const PS::TimeProfile & tp_max, const PS::S32 rank_max[], std::ostream & fout){
        PS::S32 id = 2;
        fout<<"exchange_particle= "<<tp.exchange_particle<<", max= "<<tp_max.exchange_particle<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<std::endl;
    } 

    static void dumpTimeProfile2(const PS::TimeProfile & tp, const PS::TimeProfile & tp_max, const PS::S32 rank_max[], std::ostream & fout){
        PS::S32 id = 3;
        fout<<"set_particle_local_tree= "<<tp.set_particle_local_tree<<", max= "<<tp_max.set_particle_local_tree<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"set_root_cell= "<<tp.set_root_cell<<", max= "<<tp_max.set_root_cell<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"make_local_tree= "<<tp.make_local_tree<<", max= "<<tp_max.make_local_tree<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"    morton_sort_local_tree= "<<tp.morton_sort_local_tree<<", max= "<<tp_max.morton_sort_local_tree<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"    link_cell_local_tree= "<<tp.link_cell_local_tree<<", max= "<<tp_max.link_cell_local_tree<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"calc_moment_local_tree= "<<tp.calc_moment_local_tree<<", max= "<<tp_max.calc_moment_local_tree<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"make_LET_1st= "<<tp.make_LET_1st<<", max= "<<tp_max.make_LET_1st<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"exchange_LET_1st= "<<tp.exchange_LET_1st<<", max= "<<tp_max.exchange_LET_1st<<", rank= "<<rank_max[id++]<<std::endl;

        fout<<"    exchange_LET_1st__a2a_n= "<<tp.exchange_LET_1st__a2a_n<<", max= "<<tp_max.exchange_LET_1st__a2a_n<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"    exchange_LET_1st__a2a_ep= "<<tp.exchange_LET_1st__a2a_ep<<", max= "<<tp_max.exchange_LET_1st__a2a_ep<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"    exchange_LET_1st__a2a_sp= "<<tp.exchange_LET_1st__a2a_sp<<", max= "<<tp_max.exchange_LET_1st__a2a_sp<<", rank= "<<rank_max[id++]<<std::endl;

        fout<<"make_LET_2nd= "<<tp.make_LET_2nd<<", max= "<<tp_max.make_LET_2nd<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"exchange_LET_2nd= "<<tp.exchange_LET_2nd<<", max= "<<tp_max.exchange_LET_2nd<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"set_particle_global_tree= "<<tp.set_particle_global_tree<<", max= "<<tp_max.set_particle_global_tree<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"make_global_tree= "<<tp.make_global_tree<<", max= "<<tp_max.make_global_tree<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"    morton_sort_global_tree= "<<tp.morton_sort_global_tree<<", max= "<<tp_max.morton_sort_global_tree<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"    link_cell_global_tree= "<<tp.link_cell_global_tree<<", max= "<<tp_max.link_cell_global_tree<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"calc_moment_global_tree= "<<tp.calc_moment_global_tree<<", max= "<<tp_max.calc_moment_global_tree<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<"calc_force = "<<tp.calc_force<<", max= "<<tp_max.calc_force<<", rank= "<<rank_max[id++]<<std::endl;
        fout<<std::endl;
    }

    static void getTimeProfileMax(const PS::TimeProfile & tp, const PS::S32 rank, PS::TimeProfile & tp_max, PS::S32 rank_max[]){
        PS::S32 id = 0;
        PS::Comm::getMaxValue(tp.collect_sample_particle, rank, tp_max.collect_sample_particle, rank_max[id++]);
        PS::Comm::getMaxValue(tp.decompose_domain, rank, tp_max.decompose_domain, rank_max[id++]);

        PS::Comm::getMaxValue(tp.exchange_particle, rank, tp_max.exchange_particle, rank_max[id++]);

        PS::Comm::getMaxValue(tp.set_particle_local_tree, rank, tp_max.set_particle_local_tree, rank_max[id++]);
        PS::Comm::getMaxValue(tp.set_root_cell, rank, tp_max.set_root_cell, rank_max[id++]);
        PS::Comm::getMaxValue(tp.make_local_tree, rank, tp_max.make_local_tree, rank_max[id++]);
        PS::Comm::getMaxValue(tp.morton_sort_local_tree, rank, tp_max.morton_sort_local_tree, rank_max[id++]);
        PS::Comm::getMaxValue(tp.link_cell_local_tree, rank, tp_max.link_cell_local_tree, rank_max[id++]);
        PS::Comm::getMaxValue(tp.calc_moment_local_tree, rank, tp_max.calc_moment_local_tree, rank_max[id++]);
        PS::Comm::getMaxValue(tp.make_LET_1st, rank, tp_max.make_LET_1st, rank_max[id++]);
        PS::Comm::getMaxValue(tp.exchange_LET_1st, rank, tp_max.exchange_LET_1st, rank_max[id++]);

        PS::Comm::getMaxValue(tp.exchange_LET_1st__a2a_n, rank, tp_max.exchange_LET_1st__a2a_n, rank_max[id++]);
        PS::Comm::getMaxValue(tp.exchange_LET_1st__a2a_ep, rank, tp_max.exchange_LET_1st__a2a_ep, rank_max[id++]);
        PS::Comm::getMaxValue(tp.exchange_LET_1st__a2a_sp, rank, tp_max.exchange_LET_1st__a2a_sp, rank_max[id++]);

        PS::Comm::getMaxValue(tp.make_LET_2nd, rank, tp_max.make_LET_2nd, rank_max[id++]);
        PS::Comm::getMaxValue(tp.exchange_LET_2nd, rank, tp_max.exchange_LET_2nd, rank_max[id++]);
        PS::Comm::getMaxValue(tp.set_particle_global_tree, rank, tp_max.set_particle_global_tree, rank_max[id++]);
        PS::Comm::getMaxValue(tp.make_global_tree, rank, tp_max.make_global_tree, rank_max[id++]);
        PS::Comm::getMaxValue(tp.morton_sort_global_tree, rank, tp_max.morton_sort_global_tree, rank_max[id++]);
        PS::Comm::getMaxValue(tp.link_cell_global_tree, rank, tp_max.link_cell_global_tree, rank_max[id++]);
        PS::Comm::getMaxValue(tp.calc_moment_global_tree, rank, tp_max.calc_moment_global_tree, rank_max[id++]);
        PS::Comm::getMaxValue(tp.calc_force, rank, tp_max.calc_force, rank_max[id++]);
    }
    
    Profile(Tdinfo  * _dinfo, 
            Tsystem  * _system, 
            Ttree * _tree, 
            const PS::F64 _n_op_ep_ep, 
            const PS::F64 _n_op_ep_sp, 
            const PS::F64 _flops_per_core){
        dinfo_ = _dinfo;
        system_ = _system;
        tree_ = _tree;
        n_op_ep_ep_ = _n_op_ep_ep;
        n_op_ep_sp_ = _n_op_ep_sp;
        flops_per_core_ = _flops_per_core;
    }

    void dump(std::ofstream & fout, const PS::F64 time_sys, const PS::S64 n_loop, const PS::F64 wtime_tot){
        //static PS::S64 n_loop_old = 0;
        //const PS::S64 dn_loop = n_loop - n_loop_old;
        const PS::S64 dn_loop = n_loop;
        PS::TimeProfile tp_dinfo = dinfo_->getTimeProfile();
        PS::TimeProfile tp_system = system_->getTimeProfile();
        PS::TimeProfile tp_tree = tree_->getTimeProfile();
        PS::TimeProfile tp_dinfo_max, tp_system_max, tp_tree_max;
        const PS::S32 n_profile_max = 100; 
        PS::S32 rank_dinfo_max[n_profile_max], rank_system_max[n_profile_max], rank_tree_max[n_profile_max];
        getTimeProfileMax(tp_dinfo, PS::Comm::getRank(), tp_dinfo_max, rank_dinfo_max);
        getTimeProfileMax(tp_system, PS::Comm::getRank(), tp_system_max, rank_system_max);
        getTimeProfileMax(tp_tree, PS::Comm::getRank(), tp_tree_max, rank_tree_max);
        PS::CountT n_int_ep_ep = tree_->getNumberOfInteractionEPEPGlobal();
        PS::CountT n_int_ep_sp = tree_->getNumberOfInteractionEPSPGlobal();
        PS::CountT n_op_tot = n_int_ep_ep * n_op_ep_ep_ + n_int_ep_sp * n_op_ep_sp_;
        fout<<"soft part break down"<<std::endl;
        fout<<"time_sys= "<<time_sys<<" n_loop= "<<n_loop<<std::endl;
	fout<<"n_loc= "<<system_->getNumberOfParticleLocal()<<" n_glb= "<<system_->getNumberOfParticleGlobal()<<std::endl;
        fout<<"speed= "<<(PS::F64)(n_op_tot)/(wtime_tot)*1e-12<<"[Tflops]"<<std::endl;
        fout<<"PS::Comm::getNumberOfThread()= "<<PS::Comm::getNumberOfThread()<<std::endl;
        fout<<"efficiency= "<<(PS::F64)(n_op_tot)/(wtime_tot)/(flops_per_core_*PS::Comm::getNumberOfProc()*PS::Comm::getNumberOfThread())<<std::endl;
        fout<<"wtime_tot= "<<wtime_tot<<std::endl;
        fout<<"n_op_tot= "<<n_op_tot<<std::endl;
        //timer.dump(fout);
        dumpTimeProfile0(tp_dinfo, tp_dinfo_max, rank_dinfo_max, fout);
        dumpTimeProfile1(tp_system, tp_system_max, rank_system_max, fout);
        fout<<"n_int_ep_ep= "<<n_int_ep_ep<<" n_int_ep_sp= "<<n_int_ep_sp<<std::endl;
        fout<<"ni_ave= "<<(PS::F64)(system_->getNumberOfParticleGlobal() * dn_loop) / tree_->getNumberOfWalkGlobal()
            <<" nj_ave(EP-EP)= "<<(PS::F64)(n_int_ep_ep) / (system_->getNumberOfParticleGlobal() * dn_loop)
            <<" nj_ave(EP-SP)= "<<(PS::F64)(n_int_ep_sp) / (system_->getNumberOfParticleGlobal() * dn_loop)<<std::endl;
        dumpTimeProfile2(tp_tree, tp_tree_max, rank_tree_max, fout);
        //n_loop_old = n_loop;
    }

    void clear(){
        dinfo_->clearTimeProfile();
        system_->clearTimeProfile();
	tree_->clearCounterAll();
        //tree_->clearTimeProfile();
        //tree_->clearNumberOfInteraction();
    }

};
*/

struct Tprofile{
    PS::F64 time;
    PS::F64 tbar; // barrier time, measure before barrier near end()
    const char* name;
    
    Tprofile(const char* _name): time(0.0), tbar(0.0), name(_name) {}
    
    void start(){
        time -= PS::GetWtime();
    }

    void barrier(){
        tbar -= PS::GetWtime();
    }

    void end(){
        tbar += PS::GetWtime();
        time += PS::GetWtime();
    }
    
    void print(std::ostream & fout, const PS::S32 divider=1){
        fout<<name<<": "<<time/divider<<" "<<tbar/divider<<std::endl;
    }

    void dump(std::ostream & fout, const PS::S32 width=20, const PS::S32 divider=1) const {
        fout<<std::setw(width)<<time/divider;
    }

    void dumpBarrier(std::ostream & fout, const PS::S32 width=20, const PS::S32 divider=1) const{
        fout<<std::setw(width)<<tbar/divider;
    }

    PS::F64 getMax() {
        return PS::Comm::getMaxValue(time-tbar);
    }

    PS::F64 getMin() {
        return PS::Comm::getMinValue(time-tbar);
    }

    void dumpName(std::ostream & fout, const PS::S32 width=20) const {
        fout<<std::setw(width)<<name;
    }

    void dumpBarrierName(std::ostream & fout, const PS::S32 width=20) const {
        std::string sname("Bar_");
        sname += name;
        fout<<std::setw(width)<<sname;
    }

    void reset() {
        time = 0.0;
        tbar = 0.0;
    }

};

struct NumCounter{
    PS::S64 n;
    const char* name;
    
    NumCounter(const char* _name): n(0), name(_name) {}

    NumCounter &operator++() {
        (this->n)++;
        return *this;
    }

    NumCounter &operator+=(const PS::S64 _n) {
        this->n += _n;
        return *this;
    }

    NumCounter &operator=(const PS::S64 _n) {
        this->n = _n;
        return *this;
    }

    void print(std::ostream & fout, const PS::S32 divider=1) const{
        fout<<name<<": "<<((divider==1)?n:(PS::F64)n/divider)<<std::endl;
    }

    void dump(std::ostream & fout, const PS::S32 width=20, const PS::S32 divider=1) const {
        fout<<std::setw(width)<<((divider==1)?n:(PS::F64)n/divider);
    }

    void dumpName(std::ostream & fout, const PS::S32 width=20) const {
        fout<<std::setw(width)<<name;
    }
};

class FDPSProfile: public PS::TimeProfile {
public:
    FDPSProfile &operator+=(const PS::TimeProfile _tp) {
        *(TimeProfile*)this = *(TimeProfile*)this + _tp;
        return *this;
    }

    void dumpName(std::ostream & fout, const PS::S32 width=10) const {
        fout<<std::setw(width)<<"collect_sam_ptcl"
            <<std::setw(width)<<"decompose_domain"
            <<std::setw(width)<<"exchange_ptcl"
            <<std::setw(width)<<"make_local_tree"
            <<std::setw(width)<<"make_global_tree"
            <<std::setw(width)<<"calc_force"
            <<std::setw(width)<<"calc_mom_loc_tree"
            <<std::setw(width)<<"calc_mom_gb_tree"
            <<std::setw(width)<<"make_LET_1st"
            <<std::setw(width)<<"make_LET_2nd"
            <<std::setw(width)<<"exchange_LET_1st"
            <<std::setw(width)<<"exchange_LET_2nd";
//        fout<<std::setw(width)<<"col_s_p"
//            <<std::setw(width)<<"deco-dom"
//            <<std::setw(width)<<"ex_p"
//            <<std::setw(width)<<"mk_loc_tr"
//            <<std::setw(width)<<"mk_gb_tr"
//            <<std::setw(width)<<"Force"
//            <<std::setw(width)<<"M_loc_tr"
//            <<std::setw(width)<<"M_gb_tr"
//            <<std::setw(width)<<"mk_LET_1"
//            <<std::setw(width)<<"mk_LET_2"
//            <<std::setw(width)<<"ex_LET_1"
//            <<std::setw(width)<<"ex_LET_2";
    }
    
    void dump(std::ostream & fout, const PS::S32 width=10, const PS::S64 n_loop=1){
        fout<<std::setw(width)<<collect_sample_particle/n_loop 
            <<std::setw(width)<<decompose_domain       /n_loop 
            <<std::setw(width)<<exchange_particle      /n_loop 
            <<std::setw(width)<<make_local_tree        /n_loop 
            <<std::setw(width)<<make_global_tree       /n_loop 
            <<std::setw(width)<<calc_force             /n_loop 
            <<std::setw(width)<<calc_moment_local_tree /n_loop 
            <<std::setw(width)<<calc_moment_global_tree/n_loop 
            <<std::setw(width)<<make_LET_1st           /n_loop 
            <<std::setw(width)<<make_LET_2nd           /n_loop 
            <<std::setw(width)<<exchange_LET_1st       /n_loop 
            <<std::setw(width)<<exchange_LET_2nd       /n_loop; 
    }
};

class SysProfile{
public:
	Tprofile total;
	Tprofile hard_single;	   
	Tprofile hard_isolated;
	Tprofile hard_connected;
    Tprofile hard_interrupt;
	Tprofile tree_nb;
    Tprofile tree_soft;
    Tprofile force_correct;
    Tprofile kick;
	Tprofile search_cluster;
    Tprofile create_group;
    Tprofile domain;
    Tprofile exchange;
    Tprofile output;
    Tprofile status;
    Tprofile other;
    const PS::S32 n_profile;
    
    SysProfile(): total         (Tprofile("Total          ")),
                  hard_single   (Tprofile("Hard_single    ")),
                  hard_isolated (Tprofile("Hard_isolated  ")),
                  hard_connected(Tprofile("Hard_connected ")),
                  hard_interrupt(Tprofile("Hard_interrupt*")),
                  tree_nb       (Tprofile("Tree_neighbor  ")),
                  tree_soft     (Tprofile("Tree_force     ")),
                  force_correct (Tprofile("Force_correct  ")),
                  kick          (Tprofile("Kick           ")),
                  search_cluster(Tprofile("Search_cluster ")),
                  create_group  (Tprofile("Create_group   ")),
                  domain        (Tprofile("Domain_decomp  ")),
                  exchange      (Tprofile("Exchange_ptcl  ")),
                  output        (Tprofile("Output         ")),
                  status        (Tprofile("Status         ")),
                  other         (Tprofile("Other          ")),
                  n_profile(16) {}

	void print(std::ostream & fout, const PS::F64 time_sys, const PS::S64 n_loop=1){
        fout<<"Time: "<<time_sys<<std::endl;
        
        for(PS::S32 i=0; i<n_profile; i++) {
            Tprofile* iptr = (Tprofile*)this+i;
            iptr->print(fout, n_loop);
        }
    }

    
    void dump(std::ostream & fout, const PS::S32 width=20, const PS::S64 n_loop=1) const {
        for(PS::S32 i=0; i<n_profile; i++) {
            Tprofile* iptr = (Tprofile*)this+i;
            iptr->dump(fout, width, n_loop);
        }
    }

    void dumpName(std::ostream & fout, const PS::S32 width=20) const {
        for(PS::S32 i=0; i<n_profile; i++) {
            Tprofile* iptr = (Tprofile*)this+i;
            iptr->dumpName(fout, width);
        }
    }

    void dumpBarrier(std::ostream & fout, const PS::S32 width=20, const PS::S64 n_loop=1) const {
        for(PS::S32 i=0; i<n_profile; i++) {
            Tprofile* iptr = (Tprofile*)this+i;
            iptr->dumpBarrier(fout, width, n_loop);
        }
    }

    void dumpBarrierName(std::ostream & fout, const PS::S32 width=20) const {
        for(PS::S32 i=0; i<n_profile; i++) {
            Tprofile* iptr = (Tprofile*)this+i;
            iptr->dumpBarrierName(fout, width);
        }
    }

    SysProfile getMax() {
        SysProfile pmax;
        for(PS::S32 i=0; i<n_profile; i++) {
            Tprofile* iptr = (Tprofile*)this+i;
            Tprofile* pptr = (Tprofile*)&pmax+i;
            pptr->time = iptr->getMax();
        }
        return pmax;
    }

    SysProfile getMin() {
        SysProfile pmin;
        for(PS::S32 i=0; i<n_profile; i++) {
            Tprofile* iptr = (Tprofile*)this+i;
            Tprofile* pptr = (Tprofile*)&pmin+i;
            pptr->time = iptr->getMin();
        }
        return pmin;
    }

    void clear(){
        for(PS::S32 i=0; i<n_profile; i++) {
            Tprofile* iptr = (Tprofile*)this+i;
            iptr->reset();
        }
    }

};

class SysCounts{
public:
    NumCounter hard_single;
    NumCounter hard_isolated;
    NumCounter hard_connected;
    NumCounter hard_interrupt;
    NumCounter cluster_isolated;
    NumCounter cluster_connected;
    NumCounter ARC_substep_sum;
    NumCounter ARC_tsyn_step_sum;
    NumCounter ARC_n_groups;
    NumCounter ARC_n_groups_iso;
    NumCounter H4_step_sum;
    NumCounter n_neighbor_zero;
    NumCounter ep_ep_interact;
    NumCounter ep_sp_interact;
    //NumCounter ARC_step_group;
    const PS::S32 n_counter;
    std::map<PS::S32,PS::S32> n_cluster; ///<Histogram of number of particles in clusters

    SysCounts(): hard_single      (NumCounter("Hard_single   ")),
                 hard_isolated    (NumCounter("Hard_isolated ")),
                 hard_connected   (NumCounter("Hard_connected")),
                 hard_interrupt   (NumCounter("Hard_interrupt")),
                 cluster_isolated (NumCounter("Cluster_isolated ")),
                 cluster_connected(NumCounter("Cluster_connected")),
                 ARC_substep_sum  (NumCounter("AR_step_sum")),
                 ARC_tsyn_step_sum(NumCounter("AR_tsyn_step_sum")),
                 ARC_n_groups     (NumCounter("AR_group_number")),
                 ARC_n_groups_iso (NumCounter("Iso_group_number")),
                 H4_step_sum      (NumCounter("Hermite_step_sum")),
                 n_neighbor_zero  (NumCounter("Hermite_no_NB")),
                 ep_ep_interact   (NumCounter("Ep-Ep_interaction")),
                 ep_sp_interact   (NumCounter("Ep-Sp_interaction")),
                 //ARC_step_group   (NumCounter("ARC step per group")),
                 n_counter(14) {}

    void clusterCount(const PS::S32 n, const PS::S32 ntimes=1) {
        if (n_cluster.count(n)) n_cluster[n] += ntimes;
        else n_cluster[n]=ntimes;
    }

    void clearClusterCount() {
        n_cluster.clear();
    }

    void getherClusterCount(int* n, int* count, const long unsigned int size) {
        assert(size==n_cluster.size());
        int index=0;
        for(auto i=n_cluster.begin(); i!=n_cluster.end(); ++i) {
            n[index]=i->first;
            count[index] = i->second;
            index++;
        }
    }

    void copyClusterCount(SysCounts& n_count) {
        for(auto i=n_count.n_cluster.begin(); i!=n_count.n_cluster.end(); ++i) {
            n_cluster[i->first] = i->second;
        }
    }

    void addClusterCount(SysCounts& n_count) {
        for(auto i=n_count.n_cluster.begin(); i!=n_count.n_cluster.end(); ++i) {
            if(n_cluster.count(i->first)) n_cluster[i->first] += i->second;
            else n_cluster[i->first] = i->second;
        }
    }

    void printHist(std::ostream & fout, const PS::S32 width=20, const PS::S64 n_loop=1) const {
        for(auto i=n_cluster.begin(); i!=n_cluster.end(); ++i) fout<<std::setw(width)<<i->first;
        fout<<std::endl;
        for(auto i=n_cluster.begin(); i!=n_cluster.end(); ++i) fout<<std::setw(width)<<i->second/((n_loop==1)?1:(PS::F64)n_loop);
        fout<<std::endl;
    }

    //void dump(std::ofstream & fout, const PS::S32 width=20, const PS::S64 n_loop=1){
    //    for(PS::S32 i=0; i<n_counter; i++) {
    //        NumCounter* iptr = (NumCounter*)this+i;
    //        iptr->dump(fout, width, n_loop);
    //    }
    //}

    //void dumpHist(std::ofstream & fout, const PS::S32 width=20, const PS::S64 n_loop=1){
    //    for(auto i=n_cluster.begin(); i!=n_cluster.end(); ++i)
    //        fout<<std::setw(width)<<i->first<<std::setw(width)<<i->second/((n_loop==1)?1:(PS::F64)n_loop);
    //}
    // 
    //void dumpName(std::ofstream & fout, const PS::S32 width=20) {
    //    for(PS::S32 i=0; i<n_counter; i++) {
    //        NumCounter* iptr = (NumCounter*)this+i;
    //        iptr->dumpName(fout, width);
    //    }
    //}
    
    void dump(std::ostream & fout, const PS::S32 width=20, const PS::S64 n_loop=1) const{
        for(PS::S32 i=0; i<n_counter; i++) {
            NumCounter* iptr = (NumCounter*)this+i;
            iptr->dump(fout, width, n_loop);
        }
    }


    void dumpHist(std::ostream & fout, const PS::S32 width=20, const PS::S64 n_loop=1) const{
        for(auto i=n_cluster.begin(); i!=n_cluster.end(); ++i)
            fout<<std::setw(width)<<i->first<<std::setw(width)<<i->second/((n_loop==1)?1:(PS::F64)n_loop);
    }

    void dumpName(std::ostream & fout, const PS::S32 width=20) const{
        for(PS::S32 i=0; i<n_counter; i++) {
            NumCounter* iptr = (NumCounter*)this+i;
            iptr->dumpName(fout, width);
        }
    }
    
    void clear() {
        for(PS::S32 i=0; i<n_counter; i++) {
            NumCounter* iptr = (NumCounter*)this+i;
            *iptr = 0;
        }
        n_cluster.clear();
    }
};
