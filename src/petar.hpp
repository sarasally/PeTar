#pragma once

#ifdef P3T_64BIT
#define CALC_EP_64bit
#define CALC_SP_64bit
#define RSQRT_NR_EPJ_X4
#define RSQRT_NR_SPJ_X4
#elif P3T_MIXBIT
#define CALC_EP_64bit
#define RSQRT_NR_EPJ_X4
#else
#define RSQRT_NR_EPJ_X2
//#define RSQRT_NR_SPJ_X2
#endif //P3T_64BIT

#ifdef FORCE_CHECK
#define FORCE_DIRECT
#endif

#if defined(INTRINSIC_K) || defined(INTRINSIC_X86)
#define INTRINSIC
#endif

#include<iostream>
#include<iomanip>
#include<fstream>
#include<string>
#include<sstream>
//#include<unistd.h>
#include<getopt.h>
#ifdef USE_C03
#include<map>
#else //USE_C03
#include<unordered_map>
#endif //USE_C03

#ifdef MPI_DEBUG
#include <mpi.h>
// Send recv debug
int MPI_Isend(void* buffer, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* req)
{
   int ret;

   int size;
   MPI_Type_size(datatype, &size);
   std::cerr<<"MPI_ISend count "<<count<<" dest "<<dest<<" datatype "<<size<<" tag "<<tag<<std::endl;

   ret = PMPI_Isend(buffer, count, datatype, dest, tag, comm, req);

   return ret;
}

// Debug
int MPI_Irecv(void* buffer, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* req)
{
   int ret;

   int size;
   MPI_Type_size(datatype, &size);
   std::cerr<<"MPI_IRecv count "<<count<<" dest "<<dest<<" datatype "<<size<<" tag "<<tag<<std::endl;

   ret = PMPI_Irecv(buffer, count, datatype, dest, tag, comm, req);

   return ret;
}
#endif

#include<particle_simulator.hpp>
#include"hard_assert.hpp"
#include"soft_ptcl.hpp"
#include"soft_force.hpp"
#include"energy.hpp"
#include"hard.hpp"
#include"io.hpp"
#include"status.hpp"
#include"init.hpp"
#include"integrate.hpp"
#include"domain.hpp"
#include"cluster_list.hpp"
#include"kickdriftstep.hpp"
#ifdef PROFILE
#include"profile.hpp"
#endif

//! main control class
class PeTar {
public:
#ifdef USE_QUAD
    typedef PS::TreeForForceLong<ForceSoft, EPISoft, EPJSoft>::QuadrupoleWithSymmetrySearch TreeForce; 
#else
    typedef PS::TreeForForceLong<ForceSoft, EPISoft, EPJSoft>::MonopoleWithSymmetrySearch TreeForce;
#endif
    typedef PS::ParticleSystem<FPSoft> SystemSoft;

    // For neighbor searching
    typedef PS::TreeForForceShort<ForceSoft, EPISoft, EPJSoft>::Symmetry TreeNB;

#ifdef PROFILE
    PS::S32 dn_loop;
    SysProfile profile;
    SysCounts  n_count;
    SysCounts  n_count_sum;
    PsProfile  ps_profile;
    std::ofstream fprofile;
#endif

    Status stat;
    std::ofstream fstatus;

#ifdef MAIN_DEBUG
    std::ofstream fout;
#endif

    PS::S64 n_loop;
    PS::F64 time_end;
    PS::F64 dt_tree;
    PS::F64 dt_output;
    PS::F64 domain_decompose_weight;
    PS::F64 search_cluster_radius_factor;
    std::string filename_output;
    PS::S32 output_data_format;

    FileHeader file_header;
    SystemSoft system_soft;

    PS::DomainInfo dinfo;
    PS::F64ort * pos_domain;
    TreeNB tree_nb;
    TreeForce tree_soft;
    bool init_flag;
    bool initial_step_flag;

    HardManager hard_manager;
    SystemHard system_hard_one_cluster;
    SystemHard system_hard_isolated;
    SystemHard system_hard_connected;
    PS::ReallocatableArray<PS::S32> remove_list;

    SearchCluster search_cluster;

    PS::S32 my_rank;
    PS::S32 n_proc;

    // initialization
    PeTar(): 
#ifdef PROFILE
        // profile
        dn_loop(0), profile(), n_count(), n_count_sum(), ps_profile(), fprofile(), 
#endif
        stat(), fstatus(),
#ifdef MAIN_DEBUG
        fout(),
#endif        
        n_loop(0), time_end(0.0), dt_tree(0.0), dt_output(0.0), domain_decompose_weight(1.0), search_cluster_radius_factor(0.0), filename_output(), output_data_format(0),
        file_header(), system_soft(), dinfo(), pos_domain(NULL), tree_nb(), tree_soft(), init_flag(false), initial_step_flag(true),
        hard_manager(), system_hard_one_cluster(), system_hard_isolated(), system_hard_connected(), 
        remove_list(),
        search_cluster()  {}


private:
    //! tree for neighbor searching.
    inline void treeNeighborSearch() {
#ifdef PROFILE
        profile.tree_nb.start();
#endif
#ifndef USE_SIMD
        tree_nb.calcForceAllAndWriteBack(CalcForceEpEpWithLinearCutoffNoSimd(), system_soft, dinfo);
#else
        tree_nb.calcForceAllAndWriteBack(CalcForceEpEpWithLinearCutoffSimd(), system_soft, dinfo);
#endif
        
#ifdef PROFILE
        profile.tree_nb.barrier();
        PS::Comm::barrier();
        profile.tree_nb.end();
#endif
    }

    //! search clusters
    inline void searchCluster() {
#ifdef PROFILE
        profile.search_cluster.start();
#endif
        // >2.1 search clusters ----------------------------------------
        search_cluster.searchNeighborOMP<SystemSoft, TreeNB, EPJSoft>
            (system_soft, tree_nb, pos_domain, 1.0, search_cluster_radius_factor);

        search_cluster.searchClusterLocal();
        search_cluster.setIdClusterLocal();

        // >2.2 Send/receive connect cluster
#ifdef PARTICLE_SIMULATOR_MPI_PARALLEL        
        search_cluster.connectNodes(pos_domain,tree_nb);
        search_cluster.setIdClusterGlobalIteration();
        search_cluster.sendAndRecvCluster(system_soft);
#endif

#ifdef PROFILE
        profile.search_cluster.barrier();
        PS::Comm::barrier();
        profile.search_cluster.end();
#endif    
    }

    //! and ground and create artificial particles
    inline void createGroup() {
#ifdef PROFILE
        profile.create_group.start();
#endif

        // record real particle n_loc/glb
        stat.n_real_loc = system_soft.getNumberOfParticleLocal();
        stat.n_real_glb = system_soft.getNumberOfParticleGlobal();


        // >2.3 Find ARC groups and create artificial particles
        // Set local ptcl_hard for isolated  clusters
        system_hard_isolated.setPtclForIsolatedMultiCluster(system_soft, search_cluster.adr_sys_multi_cluster_isolated_, search_cluster.n_ptcl_in_multi_cluster_isolated_);

//#ifdef CLUSTER_DEBUG
//        for (PS::S32 i=0; i<n_loc; i++) system_soft[i].status = -1000000;
//#endif

        // Find groups and add artificial particles to global particle system
        system_hard_isolated.findGroupsAndCreateArtificialParticlesOMP<SystemSoft, FPSoft>(system_soft, dt_tree);

//#ifdef CLUSTER_DEBUG
//        not correct check, isolated clusters are not reset above
//        for (PS::S32 i=0; i<n_loc; i++) {
//            if (system_soft[i].status ==-1000000) {
//                std::cerr<<"Find un initialized status i="<<i<<" status="<<system_soft[i].status<<std::endl;
//            }
//        }
//#endif
        // update n_loc_iso, n_glb_iso for isolated clusters
        //PS::S64 n_loc_iso = system_soft.getNumberOfParticleLocal();
        //PS::S64 n_glb_iso = system_soft.getNumberOfParticleGlobal();

#ifdef PARTICLE_SIMULATOR_MPI_PARALLEL
        // For connected clusters
        system_hard_connected.setPtclForConnectedCluster(system_soft, search_cluster.mediator_sorted_id_cluster_, search_cluster.ptcl_recv_);
        system_hard_connected.findGroupsAndCreateArtificialParticlesOMP<SystemSoft, FPSoft>(system_soft, dt_tree);
        // send updated particle back to original (set zero mass particle to origin)
        search_cluster.writeAndSendBackPtcl(system_soft, system_hard_connected.getPtcl(), remove_list);
#endif

        // update n_glb, n_glb for all
        stat.n_all_loc = system_soft.getNumberOfParticleLocal();
        stat.n_all_glb = system_soft.getNumberOfParticleGlobal();
            
        // >2.4 set adr/rank for artificial particles in GPS
#pragma omp parallel for
        for(PS::S32 i=stat.n_real_loc; i<stat.n_all_loc; i++){
            system_soft[i].rank_org = my_rank;
            system_soft[i].adr = i;
#ifdef HARD_DEBUG
            assert(system_soft[i].status.d>0);
#endif
        }
        // >3 Tree for force ----------------------------------------
#ifdef PROFILE
        profile.create_group.barrier();
        PS::Comm::barrier();
        profile.create_group.end();
#endif
    }

    //! calculate tree solf force
    inline void treeSoftForce() {
#ifdef PROFILE
        profile.tree_soft.start();

        tree_soft.clearNumberOfInteraction();
        tree_soft.clearTimeProfile();
#endif

#ifndef USE_SIMD
        tree_soft.calcForceAllAndWriteBack(CalcForceEpEpWithLinearCutoffNoSimd(),
#ifdef USE_QUAD
                                           CalcForceEpSpQuadNoSimd(),
#else
                                           CalcForceEpSpMonoNoSimd(),
#endif
                                           system_soft,
                                           dinfo);
#else
        tree_soft.calcForceAllAndWriteBack(CalcForceEpEpWithLinearCutoffSimd(),
#ifdef USE_QUAD
                                           CalcForceEpSpQuadSimd(),
#else
                                           CalcForceEpSpMonoSimd(),
#endif
                                           system_soft,
                                           dinfo);
#endif

#ifdef PROFILE
        n_count.ep_ep_interact     += tree_soft.getNumberOfInteractionEPEPLocal();
        n_count_sum.ep_ep_interact += tree_soft.getNumberOfInteractionEPEPGlobal();
        n_count.ep_sp_interact     += tree_soft.getNumberOfInteractionEPSPLocal();
        n_count_sum.ep_sp_interact += tree_soft.getNumberOfInteractionEPSPGlobal(); 

        ps_profile += tree_soft.getTimeProfile();
        domain_decompose_weight = ps_profile.calc_force;

        profile.tree_soft.barrier();
        PS::Comm::barrier();
        profile.tree_soft.end();
#endif
    }

    // correct force due to change over function
    inline void treeForceCorrectChangeover() {
#ifdef PROFILE
        profile.force_correct.start();
#endif

#ifdef CORRECT_FORCE_DEBUG
        // backup particle data
        PS::S64 n_loc_all = system_soft.getNumberOfParticleLocal();

        FPSoft psys_bk[n_loc_all];
#pragma omp parallel for
        for (int i=0; i<n_loc_all; i++) 
            psys_bk[i] = system_soft[i];
#endif

        // single 
        system_hard_one_cluster.correctPotWithCutoffOMP(system_soft, search_cluster.getAdrSysOneCluster());

        // Isolated clusters
        system_hard_isolated.correctForceWithCutoffClusterOMP(system_soft);

#ifdef PARTICLE_SIMULATOR_MPI_PARALLEL        
        // Connected clusters
        system_hard_connected.correctForceWithCutoffTreeNeighborAndClusterOMP<SystemSoft, FPSoft, TreeForce, EPJSoft>(system_soft, tree_soft, search_cluster.getAdrSysConnectClusterSend());
#endif

#ifdef CORRECT_FORCE_DEBUG
        // switch backup and sys particle data
#pragma omp parallel for
        for (int i=0; i<n_loc_all; i++) {
            FPSoft ptmp = psys_bk[i];
            psys_bk[i] = system_soft[i];
            system_soft[i] = ptmp;
        }

        // all particles
        system_hard_isolated.correctForceWithCutoffTreeNeighborOMP<SystemSoft, FPSoft, TreeForce, EPJSoft>(system_soft, tree_soft, n_loc);

        // single 
        //system_hard_one_cluster.correctPotWithCutoffOMP(system_soft, search_cluster.getAdrSysOneCluster());
        // Isolated clusters
        //system_hard_isolated.correctForceWithCutoffTreeNeighborOMP<SystemSoft, FPSoft, TreeForce, EPJSoft>(system_soft, tree_soft, n_loc);
//#ifdef PARTICLE_SIMULATOR_MPI_PARALLEL        
//        // Connected clusters
//        system_hard_connected.correctForceWithCutoffTreeNeighborOMP<SystemSoft, FPSoft, TreeForce, EPJSoft>(system_soft, tree_soft, n_loc_all);
//#endif

        for (int i=0; i<n_loc_all; i++) {
            PS::F64vec dacci=psys_bk[i].acc- system_soft[i].acc;
            PS::F64 dpoti = psys_bk[i].pot_tot- system_soft[i].pot_tot;
            if(dacci*dacci/(system_soft[i].acc*system_soft[i].acc)>1e-8) {
                std::cerr<<"Corrected Acc diff >1e-8: i "<<i<<" acc(tree): "<<system_soft[i].acc<<" acc(cluster): "<<psys_bk[i].acc<<std::endl;
                abort();
            }
            // Notice, the cluster members can include particles are not in the tree neighbor searching. If the extra neighbors in clusters are group members. The correct from clusters will be more accurate than tree neighbor correction. Since the potential is calculated from the real members instead of artificial particles. This can result in different potential
            if(abs(dpoti/system_soft[i].pot_tot)>1e-6) {
                std::cerr<<"Corrected pot diff >1e-6: i "<<i<<" pot(tree): "<<system_soft[i].pot_tot<<" pot(cluster): "<<psys_bk[i].pot_tot<<std::endl;
                abort();
            }
        }
        
#endif
#ifdef PROFILE
        profile.force_correct.barrier();
        PS::Comm::barrier();
        profile.force_correct.end();
#endif
    }

#ifdef KDKDK_4TH
    //! gradient kick for KDKDK_4TH method
    inline void GradientKick() {
#ifdef PROFILE
        profile.tree_soft.start();
        tree_soft.clearNumberOfInteraction();
        tree_soft.clearTimeProfile();
#endif
        // correction calculation
        //tree_soft.setParticaleLocalTree(system_soft, false);
        
        tree_soft.calcForceAllAndWriteBack(CalcCorrectEpEpWithLinearCutoffNoSimd(),
#ifdef USE_QUAD
                                           CalcForceEpSpQuadNoSimd(),
#else
                                           CalcForceEpSpMonoNoSimd(),
#endif
                                           system_soft,
                                           dinfo);

#ifdef PROFILE
        n_count.ep_ep_interact     += tree_soft.getNumberOfInteractionEPEPLocal();
        n_count_sum.ep_ep_interact += tree_soft.getNumberOfInteractionEPEPGlobal();
        n_count.ep_sp_interact     += tree_soft.getNumberOfInteractionEPSPLocal();
        n_count_sum.ep_sp_interact += tree_soft.getNumberOfInteractionEPSPGlobal(); 

        ps_profile += tree_soft.getTimeProfile();
        domain_decompose_weight += ps_profile.calc_force;

        profile.tree_soft.barrier();
        PS::Comm::barrier();
        profile.tree_soft.end();
        profile.force_correct.start();
#endif 

        // Isolated clusters
        system_hard_isolated.correctForceWithCutoffClusterOMP(system_soft, true);

#ifdef PARTICLE_SIMULATOR_MPI_PARALLEL        
        // Connected clusters
        system_hard_connected.correctForceWithCutoffTreeNeighborAndClusterOMP<SystemSoft, FPSoft, TreeForce, EPJSoft>(system_soft, tree_soft, search_cluster.getAdrSysConnectClusterSend(), true);
#endif

#ifdef PROFILE
        profile.force_correct.barrier();
        PS::Comm::barrier();
        profile.force_correct.end();
#endif
//        if (true) {


// for debug
//            FILE* fdump;
//            if ( (fdump = fopen("acorr.dump","w")) == NULL) {
//                fprintf(stderr,"Error: Cannot open file acorr.dump\n");
//                abort();
//            }
//            for (int i=0; i<n_loc; i++) {
//                fprintf(fdump, "%d %.12g %.12g %.12g %.12g %.12g %.12g\n", i, system_soft[i].acc[0], system_soft[i].acc[1], system_soft[i].acc[2], system_soft[i].acorr[0], system_soft[i].acorr[1], system_soft[i].acorr[2]);
//            }
//            fclose(fdump);
//            abort();
// debug
//            }

    }
#endif

    //! kick
    inline void kick(const PS::F64 _dt_kick) {
#ifdef PROFILE
        profile.kick.start();
#endif
        /// Member mass are recovered
        // single and reset status to zero (due to binary disruption)
        kickOne(system_soft, _dt_kick, search_cluster.getAdrSysOneCluster());
        // isolated
        kickClusterAndRecoverGroupMemberMass(system_soft, system_hard_isolated.getPtcl(), _dt_kick);
        // c.m. artificial
        kickCM(system_soft, stat.n_real_loc, hard_manager.ap_manager, _dt_kick);
        
#ifdef PARTICLE_SIMULATOR_MPI_PARALLEL
        // connected
        kickClusterAndRecoverGroupMemberMass(system_soft, system_hard_connected.getPtcl(), _dt_kick);
        // sending list for connected clusters
        kickSend(system_soft, search_cluster.getAdrSysConnectClusterSend(), _dt_kick);
        // send kicked particle from sending list, and receive remote single particle
        search_cluster.SendSinglePtcl(system_soft, system_hard_connected.getPtcl());
#endif

#ifdef HARD_DEBUG
        PS::S32 kick_regist[stat.n_real_loc];
        for(int i=0; i<stat.n_real_loc; i++) kick_regist[i] = 0;
        for(int i=0; i<search_cluster.getAdrSysOneCluster().size(); i++) {
            kick_regist[search_cluster.getAdrSysOneCluster()[i]]++;
        }
        for(int i=0; i<system_hard_isolated.getPtcl().size(); i++) {
            PS::S64 adr= system_hard_isolated.getPtcl()[i].adr_org;
            assert(adr>=0);
            kick_regist[adr]++;
        }
        for(int i=0; i<system_hard_connected.getPtcl().size(); i++) {
            PS::S64 adr= system_hard_connected.getPtcl()[i].adr_org;
            if(adr>=0) kick_regist[adr]++;
        }
        for(int i=0; i<search_cluster.getAdrSysConnectClusterSend().size(); i++) {
            PS::S64 adr= search_cluster.getAdrSysConnectClusterSend()[i];
            kick_regist[adr]++;
        }
        for(int i=0; i<stat.n_real_loc; i++) {
            assert(kick_regist[i]==1);
        }
#endif

#ifdef PROFILE
        profile.kick.barrier();
        PS::Comm::barrier();
        profile.kick.end();
#endif
    }

    //! hard drift
    inline void drift(const PS::F64 _dt_drift) {
        ////// set time
        system_hard_one_cluster.setTimeOrigin(stat.time);
        system_hard_isolated.setTimeOrigin(stat.time);
        system_hard_connected.setTimeOrigin(stat.time);
        ////// set time

        // reset slowdown energy correction
        system_hard_isolated.energy.resetEnergyCorrection();
        system_hard_connected.energy.resetEnergyCorrection();
        
#ifdef PROFILE
        profile.hard_single.start();
#endif
        ////// integrater one cluster
        system_hard_one_cluster.initializeForOneCluster(search_cluster.getAdrSysOneCluster().size());
        system_hard_one_cluster.setPtclForOneCluster(system_soft, search_cluster.getAdrSysOneCluster());
        system_hard_one_cluster.driveForOneClusterOMP(_dt_drift);
        //system_hard_one_cluster.writeBackPtclForOneClusterOMP(system_soft, search_cluster.getAdrSysOneCluster());
        system_hard_one_cluster.writeBackPtclForOneClusterOMP(system_soft);
        ////// integrater one cluster
#ifdef PROFILE
        profile.hard_single.barrier();
        profile.hard_single.end();
#endif
        ////////////////

        /////////////
#ifdef PROFILE
        profile.hard_isolated.start();
#endif
        // integrate multi cluster A
        system_hard_isolated.driveForMultiClusterOMP(_dt_drift, &(system_soft[0]));
        //system_hard_isolated.writeBackPtclForMultiCluster(system_soft, search_cluster.adr_sys_multi_cluster_isolated_,remove_list);
        system_hard_isolated.writeBackPtclForMultiCluster(system_soft, remove_list);
        // integrate multi cluster A
#ifdef PROFILE
        profile.hard_isolated.barrier();
        profile.hard_isolated.end();
#endif
        /////////////

#ifdef PARTICLE_SIMULATOR_MPI_PARALLEL        
        /////////////
#ifdef PROFILE
        profile.hard_connected.start();
#endif
        // integrate multi cluster B
        system_hard_connected.driveForMultiClusterOMP(_dt_drift, &(system_soft[0]));
        search_cluster.writeAndSendBackPtcl(system_soft, system_hard_connected.getPtcl(), remove_list);
        // integrate multi cluster B
#ifdef PROFILE
        profile.hard_connected.barrier();
        PS::Comm::barrier();
        profile.hard_connected.end();
#endif

#endif

    }
    
    //! remove artificial and unused particles
    inline void removeParticles() {
        /////////////
        // Remove ghost particles
        system_soft.removeParticle(remove_list.getPointer(), remove_list.size());
        // reset particle number
        stat.n_real_loc = stat.n_real_loc-remove_list.size();
        system_soft.setNumberOfParticleLocal(stat.n_real_loc);
        stat.n_real_glb = system_soft.getNumberOfParticleGlobal();
        remove_list.resizeNoInitialize(0);
    }

    //! correct force due to the change over update
    inline void correctForceChangeOverUpdate() {
#ifdef PROFILE
        profile.force_correct.start();
#endif
        // correct changeover for first step
        // Isolated clusters
        system_hard_isolated.correctForceForChangeOverUpdateOMP<SystemSoft, TreeForce, EPJSoft>(system_soft, tree_soft);

#ifdef PARTICLE_SIMULATOR_MPI_PARALLEL        
        // Connected clusters
        auto& adr_send = search_cluster.getAdrSysConnectClusterSend();
        system_hard_connected.correctForceForChangeOverUpdateOMP<SystemSoft, TreeForce, EPJSoft>(system_soft, tree_soft, adr_send.getPointer(), adr_send.size());
#endif

#ifdef PROFILE
        profile.force_correct.barrier();
        PS::Comm::barrier();
        profile.force_correct.end();
#endif

    }

    //! domain decomposition
    inline void domainDecompose() {
#ifdef PROFILE
        // > 6. Domain decomposition
        profile.domain.start();
#endif
        // Domain decomposition, parrticle exchange and force calculation
        if(n_loop % 16 == 0) {
            dinfo.decomposeDomainAll(system_soft,domain_decompose_weight);
            //std::cout<<"rank: "<<my_rank<<" weight: "<<domain_decompose_weight<<std::endl;
        }
#ifdef PROFILE
        profile.domain.barrier();
        PS::Comm::barrier();
        profile.domain.end();
#endif
    }

    //! exchange particles
    inline void exchangeParticle() {
#ifdef PROFILE
        profile.exchange.start();
#endif
        system_soft.exchangeParticle(dinfo);

        const PS::S32 n_loc_tmp = system_soft.getNumberOfParticleLocal();
#pragma omp parallel for
        for(PS::S32 i=0; i<n_loc_tmp; i++){
            system_soft[i].rank_org = my_rank;
            system_soft[i].adr = i;
        }
        
#ifdef PROFILE
        profile.exchange.barrier();
        PS::Comm::barrier();
        profile.exchange.end();
#endif
    }

    //! adjust dt_reduce_factor for tree time step
    /*!
      \return true: if tree time need update
     */
    inline bool adjustDtTreeReduce(PS::F64& _dt_reduce_factor, const PS::F64 _dt_tree_base, const PS::F64 _dt_tree_now) {
        bool dt_mod_flag = false;
        PS::F64 dt_tree_new = _dt_tree_base / _dt_reduce_factor;
        if(_dt_tree_now!= dt_tree_new) {
            // in increasing case, need to make sure the time is consistent with block step time/dt_tree
            if(_dt_tree_now<dt_tree_new) {
                PS::F64 dt_tree_max = calcDtLimit(stat.time, _dt_tree_base, _dt_tree_now);
                if (dt_tree_max == _dt_tree_now) {
                    // no modificatoin, recover original reduce factor
                    _dt_reduce_factor= _dt_tree_base/_dt_tree_now;
                }
                else {
                    dt_mod_flag = true;
                    // limit dt_reduce_factor to the maximum block step allown
                    _dt_reduce_factor = std::min(_dt_tree_base/dt_tree_max, _dt_reduce_factor);

                }
            }
            else dt_mod_flag = true;
        }

        return dt_mod_flag;
    }

    inline void updateStatus(const bool _initial_flag) {

#ifdef PETAR_DEBUG
        assert(stat.n_all_loc==system_soft.getNumberOfParticleLocal());
#endif

        if (_initial_flag) {
            // calculate initial energy
            stat.energy.clear();
            stat.energy.calc(&system_soft[0], stat.n_real_loc, true);
#ifdef PARTICLE_SIMULATOR_MPI_PARALLEL
            stat.energy.getSumMultiNodes(true);
#endif
#ifdef HARD_CHECK_ENERGY
            stat.energy.ekin_sd = stat.energy.ekin;
            stat.energy.epot_sd = stat.energy.epot;
            stat.energy_hard_diff = 0;
            stat.energy_hard_sd_diff = 0;
#endif

        }
        else {

#ifdef HARD_CHECK_ENERGY
            // reset sd energy reference to no slowdown case
            stat.energy.etot_sd_ref -= stat.energy.ekin_sd + stat.energy.epot_sd - stat.energy.ekin - stat.energy.epot;
#endif

            stat.energy.calc(&system_soft[0], stat.n_real_loc);
#ifdef PARTICLE_SIMULATOR_MPI_PARALLEL
            stat.energy.getSumMultiNodes();
#endif

#ifdef HARD_CHECK_ENERGY
            HardEnergy energy_local = system_hard_isolated.energy;
            energy_local += system_hard_connected.energy;
            // hard energy error
            stat.energy_hard_diff += PS::Comm::getSum(energy_local.de);
            stat.energy_hard_sd_diff += PS::Comm::getSum(energy_local.de_sd);
            // energy correction due to slowdown 
            PS::F64 ekin_sd_correction = PS::Comm::getSum(energy_local.ekin_sd_correction);
            stat.energy.ekin_sd = stat.energy.ekin + ekin_sd_correction;
            PS::F64 epot_sd_correction = PS::Comm::getSum(energy_local.epot_sd_correction);
            stat.energy.epot_sd = stat.energy.epot + epot_sd_correction;
            PS::F64 etot_sd_correction = ekin_sd_correction + epot_sd_correction;
            // for total energy reference, first add the cumulative change due to slowdown change in the integration (referring to no slowdown case), then add slowdown energy correction from current time
            stat.energy.etot_sd_ref += PS::Comm::getSum(energy_local.de_sd_change_cum) + etot_sd_correction;

            system_hard_isolated.energy.clear();
            system_hard_connected.energy.clear();
#endif
        }
    }

    inline void output(const bool _initial_flag) {
#ifdef PROFILE
        profile.output.start();
#endif

        // print status
        if(my_rank==0) {
            std::cout<<std::endl;
            stat.print(std::cout);
            stat.printColumn(fstatus, WRITE_WIDTH);
            fstatus<<std::endl;
        }

        if (!_initial_flag) {
            // data output
            file_header.n_body = stat.n_real_glb;
            file_header.time = stat.time;
            file_header.nfile++;
            std::string fname = filename_output+"."+std::to_string(file_header.nfile);
            system_soft.setNumberOfParticleLocal(stat.n_real_loc);
            if (output_data_format==1||output_data_format==3)
                system_soft.writeParticleAscii(fname.c_str(), file_header);
            else if(output_data_format==0||output_data_format==2)
                system_soft.writeParticleBinary(fname.c_str(), file_header);
            system_soft.setNumberOfParticleLocal(stat.n_all_loc);


#ifdef MAIN_DEBUG
            write_p(fout, stat.time, system_soft, stat.energy);
#endif
        }

#ifdef PROFILE
        profile.output.barrier();
        PS::Comm::barrier();
        profile.output.end();
#endif
    }

#ifdef PROFILE
    inline void calcProfile() {
        profile.tot.barrier();
        profile.tot.end();
        
        // profile analysis

        PS::S32 n_hard_single     = system_hard_one_cluster.getPtcl().size();
        PS::S32 n_hard_isolated   = system_hard_isolated.getPtcl().size();
        PS::S32 n_hard_connected  = system_hard_connected.getPtcl().size();

        n_count.hard_single      += n_hard_single;
        n_count.hard_isolated    += n_hard_isolated;
        n_count.hard_connected   += n_hard_connected;

        n_count_sum.hard_single      += PS::Comm::getSum(n_hard_single);
        n_count_sum.hard_isolated    += PS::Comm::getSum(n_hard_isolated);
        n_count_sum.hard_connected   += PS::Comm::getSum(n_hard_connected);
                                           
        PS::S64 ARC_substep_sum   = system_hard_isolated.ARC_substep_sum;
        ARC_substep_sum += system_hard_connected.ARC_substep_sum;
        PS::S64 ARC_tsyn_step_sum   = system_hard_isolated.ARC_tsyn_step_sum;
        ARC_tsyn_step_sum += system_hard_connected.ARC_tsyn_step_sum;
        PS::S64 ARC_n_groups      = system_hard_isolated.ARC_n_groups;
        ARC_n_groups += system_hard_connected.ARC_n_groups;
        PS::S64 H4_step_sum       = system_hard_isolated.H4_step_sum;
        H4_step_sum +=  system_hard_connected.H4_step_sum;
        n_count.ARC_substep_sum  += ARC_substep_sum;
        n_count.ARC_tsyn_step_sum+= ARC_tsyn_step_sum;
        n_count.ARC_n_groups     += ARC_n_groups;
        n_count.H4_step_sum      += H4_step_sum;

        n_count_sum.ARC_substep_sum  += PS::Comm::getSum(ARC_substep_sum);
        n_count_sum.ARC_tsyn_step_sum+= PS::Comm::getSum(ARC_tsyn_step_sum);
        n_count_sum.ARC_n_groups     += PS::Comm::getSum(ARC_n_groups);
        n_count_sum.H4_step_sum      += PS::Comm::getSum(H4_step_sum);

        system_hard_isolated.ARC_substep_sum = 0;
        system_hard_isolated.ARC_tsyn_step_sum=0;
        system_hard_isolated.ARC_n_groups = 0;
        system_hard_isolated.H4_step_sum = 0;
        system_hard_connected.ARC_substep_sum = 0;
        system_hard_connected.ARC_tsyn_step_sum=0;
        system_hard_connected.ARC_n_groups = 0;
        system_hard_connected.H4_step_sum = 0;
                                           
        n_count.cluster_count(1, n_hard_single);

        const PS::S32  n_isolated_cluster = system_hard_isolated.getNCluster();
        n_count.cluster_isolated += n_isolated_cluster;
        n_count_sum.cluster_isolated += PS::Comm::getSum(n_isolated_cluster);

        const PS::S32* isolated_cluster_n_list = system_hard_isolated.getClusterNList();
        for (PS::S32 i=0; i<n_isolated_cluster; i++) n_count.cluster_count(isolated_cluster_n_list[i]);

        const PS::S32  n_connected_cluster = system_hard_connected.getNCluster();
        n_count.cluster_connected += n_connected_cluster;
        n_count_sum.cluster_connected += PS::Comm::getSum(n_connected_cluster);
        const PS::S32* connected_cluster_n_list = system_hard_connected.getClusterNList();
        for (PS::S32 i=0; i<n_connected_cluster; i++) n_count.cluster_count(connected_cluster_n_list[i]);

        dn_loop++;

        // output profile data
        if(fmod(stat.time, dt_output) == 0.0) {

            //const int NProc=PS::Comm::getNumberOfProc();

            const SysProfile& profile_min = profile.getMin();
            const SysProfile& profile_max = profile.getMax();
        
            if(my_rank==0) {
                std::cout<<std::setprecision(5);
                std::cout<<"Tree step number: "<<dn_loop
                         <<"  Local N: "<<stat.n_real_loc
                         <<"  Local Nall: "<<stat.n_all_loc<<std::endl;
                

                std::cout<<"**** Wallclock time per step (local): [Min/Max]\n";
                //std::cout<<std::setw(PRINT_WIDTH)<<"Rank";
                profile.dumpName(std::cout,PRINT_WIDTH);
                std::cout<<std::endl;

                //std::cout<<std::setw(PRINT_WIDTH)<<my_rank;
                profile_min.dump(std::cout,PRINT_WIDTH,dn_loop);
                std::cout<<std::endl;
                profile_max.dump(std::cout,PRINT_WIDTH,dn_loop);
                std::cout<<std::endl;

                std::cout<<"**** FDPS time profile (local):\n";
                ps_profile.dumpName(std::cout,PRINT_WIDTH);
                std::cout<<std::endl;
                ps_profile.dump(std::cout,PRINT_WIDTH,dn_loop);
                std::cout<<std::endl;

                std::cout<<"**** Number per step (global):\n";
                n_count_sum.dumpName(std::cout,PRINT_WIDTH);
                std::cout<<std::endl;
                n_count_sum.dump(std::cout,PRINT_WIDTH,dn_loop);
                std::cout<<std::endl;
                
                std::cout<<"**** Number of members in clusters (local):\n";
                n_count.printHist(std::cout,PRINT_WIDTH,dn_loop);
            }

            fprofile<<std::setprecision(WRITE_PRECISION);
            fprofile<<std::setw(WRITE_WIDTH)<<my_rank;
            fprofile<<std::setw(WRITE_WIDTH)<<stat.time
                    <<std::setw(WRITE_WIDTH)<<dn_loop
                    <<std::setw(WRITE_WIDTH)<<stat.n_real_loc;
            profile.dump(fprofile, WRITE_WIDTH, dn_loop);
            profile.dumpBarrier(fprofile, WRITE_WIDTH, dn_loop);
            ps_profile.dump(fprofile, WRITE_WIDTH, dn_loop);
            n_count.dump(fprofile, WRITE_WIDTH, dn_loop);
            fprofile<<std::endl;

            profile.clear();
            ps_profile.clear();
            n_count.clear();
            n_count_sum.clear();
            dn_loop=0;
        }
    }
#endif

public:
    // reading input parameters
    PS::S32 readParameters(int argc, char *argv[]) {
        init_flag = true;
        initial_step_flag = true;
        // set print format
        std::cout<<std::setprecision(PRINT_PRECISION);
        std::cerr<<std::setprecision(PRINT_PRECISION);

        // initial FPDS
        PS::Initialize(argc, argv);

        // initial parameters
        IOParamsContainer input_par_store;

        IOParams<PS::F64> ratio_r_cut  (input_par_store, 0.1,  "r_in / r_out");
        IOParams<PS::F64> theta        (input_par_store, 0.3,  "Openning angle theta");
        IOParams<PS::S32> n_leaf_limit (input_par_store, 20,   "Tree leaf number limit", "optimized value shoudl be slightly >=11+N_bin_sample (20)");
#ifdef USE__AVX512
        IOParams<PS::S32> n_group_limit(input_par_store, 1024, "Tree group number limit", "optimized for x86-AVX512 (1024)");    
#else
        IOParams<PS::S32> n_group_limit(input_par_store, 512,  "Tree group number limit", "optimized for x86-AVX2 (512)");
#endif
        IOParams<PS::S32> n_smp_ave    (input_par_store, 100,  "Average target number of sample particles per process");
        IOParams<PS::S32> n_split      (input_par_store, 8,    "Number of binary sample points for tree perturbation force");
        IOParams<PS::S64> n_bin        (input_par_store, 0,    "Number of primordial binaries (assume binaries ID=1,2*n_bin)");
        IOParams<PS::F64> time_end     (input_par_store, 10.0, "Finishing time");
        IOParams<PS::F64> eta          (input_par_store, 0.1,  "Hermite time step coefficient eta");
        IOParams<PS::S64> n_glb        (input_par_store, 16384,"Total number of particles (this will suppress reading snapshot data and use Plummer model generator without binary)");
        IOParams<PS::F64> dt_soft      (input_par_store, 0.0,  "Tree timestep","0.1*r_out/sigma_1D");
        IOParams<PS::F64> dt_snp       (input_par_store, 0.0625,"Output time interval of particle dataset");
        IOParams<PS::F64> search_factor(input_par_store, 3.0,  "Neighbor searching coefficient for v*dt");
        IOParams<PS::F64> radius_factor(input_par_store, 1.5,  "Neighbor searching radius factor for peri-center check");
        IOParams<PS::F64> dt_limit_hard_factor(input_par_store, 4.0,  "Limit of tree time step/hard time step");
        IOParams<PS::S32> dt_min_hermite_index(input_par_store, 40,   "Power index n for the smallest time step (0.5^n) allowed in Hermite integrator");
        IOParams<PS::S32> dt_min_arc_index    (input_par_store, 64,   "Power index n for the smallest time step (0.5^n) allowed in ARC integrator, suppressed");
        IOParams<PS::F64> dt_err_pert  (input_par_store, 1e-6, "Time synchronization maximum (relative) error for perturbed ARC integrator, suppressed");
        IOParams<PS::F64> dt_err_soft  (input_par_store, 1e-3, "Time synchronization maximum (relative) error for no-perturber (only soft perturbation) ARC integrator, suppressed");
        IOParams<PS::F64> e_err_arc    (input_par_store, 1e-10,"Maximum energy error allown for ARC integrator");
#ifdef HARD_CHECK_ENERGY
        IOParams<PS::F64> e_err_hard   (input_par_store, 1e-4, "Maximum energy error allown for hard integrator");
#endif
#ifdef AR_SYM
        IOParams<PS::S32> step_limit_arc(input_par_store, 1000000, "Maximum step allown for ARC sym integrator");
#endif
        IOParams<PS::F64> eps          (input_par_store, 0.0,  "Softerning eps");
        IOParams<PS::F64> r_out        (input_par_store, 0.0,  "Transit function outer boundary radius", "<m>/sigma_1D^2/ratio_r_cut");
        IOParams<PS::F64> r_bin        (input_par_store, 0.0,  "Tidal tensor box size and binary radius criterion", "theta*r_in");
        IOParams<PS::F64> r_search_max (input_par_store, 0.0,  "Maximum search radius criterion", "5*r_out");
        IOParams<PS::F64> sd_factor    (input_par_store, 1e-4, "Slowdown perturbation criterion");
        IOParams<PS::S32> data_format  (input_par_store, 1,    "Data read(r)/write(w) format BINARY(B)/ASCII(A): Writing off: r-B(5), r-A(4); Writing on: r-B/w-A (3); r-A/w-B (2); rw-A (1); rw-B (0)");
        IOParams<std::string> fname_snp(input_par_store, "data","Prefix filename of dataset: [prefix].[File ID]");
        IOParams<std::string> fname_par(input_par_store, "input.par", "Input parameter file (this option should be used first before any other options)");

        PS::F64 r_search_min=0.0;
        input_par_store.store(&r_search_min);

        my_rank = PS::Comm::getRank();
        n_proc = PS::Comm::getNumberOfProc();

        // reading parameters
        bool reading_flag=true;
        bool app_flag=false; // appending data flag

        static struct option long_options[] = {
            {"n-split", required_argument, 0, 0},        
            {"search-factor", required_argument, 0, 1},  
            {"dt-max-factor", required_argument, 0, 2},  
            {"dt-min-hermite", required_argument, 0, 3}, 
            {"energy-err-arc", required_argument, 0, 7}, 
            {"soft-eps", required_argument, 0, 8},       
            {"slowdown-factor", required_argument, 0, 9},
            {"r-ratio", required_argument, 0, 10},       
            {"r-bin",   required_argument, 0, 11},       
            {"radius_factor", required_argument, 0, 12}, 
            {"help",no_argument, 0, 'h'},                
#ifdef HARD_CHECK_ENERGY
            {"energy-err-hard", required_argument, 0, 14},  
#endif
#ifdef AR_SYM
            {"step-limit-arc", required_argument, 0, 15},   
#endif
            {0,0,0,0}
        };

        int copt;
        int option_index;
        if(my_rank == 0) std::cout<<"----- input option -----\n";
        while ((copt = getopt_long(argc, argv, "i:at:s:o:r:b:n:G:L:S:T:E:f:p:h", long_options, &option_index)) != -1) 
            switch (copt) {
            case 0:
                n_split.value = atoi(optarg);
                if(my_rank == 0) n_split.print(std::cout);
                assert(n_split.value>=8);
                break;
            case 1:
                search_factor.value = atof(optarg);
                if(my_rank == 0) search_factor.print(std::cout);
                assert(search_factor.value>0.0);
                break;
            case 2:
                dt_limit_hard_factor.value = atof(optarg);
                if(my_rank == 0) dt_limit_hard_factor.print(std::cout);
                assert(dt_limit_hard_factor.value > 0.0);
                break;
            case 3:
                dt_min_hermite_index.value = atoi(optarg);
                if(my_rank == 0) dt_min_hermite_index.print(std::cout);
                assert(dt_min_hermite_index.value > 0);
                break;
            case 7:
                e_err_arc.value = atof(optarg);
                if(my_rank == 0) e_err_arc.print(std::cout);
                assert(e_err_arc.value > 0.0);
                break;
            case 8:
                eps.value = atof(optarg);
                if(my_rank == 0) eps.print(std::cout);
                assert(eps.value>=0.0);
                break;
            case 9:
                sd_factor.value = atof(optarg);
                if(my_rank == 0) sd_factor.print(std::cout);
                assert(sd_factor.value>0.0);
                break;
            case 10:
                ratio_r_cut.value = atof(optarg);
                if(my_rank == 0) ratio_r_cut.print(std::cout);
                assert(ratio_r_cut.value>0.0);
                assert(ratio_r_cut.value<1.0);
                break;
            case 11:
                r_bin.value = atof(optarg);
                if(my_rank == 0) r_bin.print(std::cout);
                assert(r_bin.value>0.0);
                break;
            case 12:
                radius_factor.value = atof(optarg);
                if(my_rank == 0) radius_factor.print(std::cout);
                assert(radius_factor.value>=1.0);
                break;
#ifdef HARD_CHECK_ENERGY
            case 14:
                e_err_hard.value = atof(optarg);
                if(my_rank == 0) e_err_hard.print(std::cout);
                break;
#endif
#ifdef AR_SYM
            case 15:
                step_limit_arc.value = atoi(optarg);
                if(my_rank == 0) step_limit_arc.print(std::cout);
                break;
#endif
            case 'i':
                data_format.value = atoi(optarg);
                if(my_rank == 0) data_format.print(std::cout);
                assert(data_format.value>=0||data_format.value<=3);
                break;
            case 'a':
                app_flag=true;
                break;
            case 't':
                time_end.value = atof(optarg);
                if(my_rank == 0) time_end.print(std::cout);
                assert(time_end.value>=0.0);
                break;
            case 's':
                dt_soft.value = atof(optarg);
                if(my_rank == 0) dt_soft.print(std::cout);
                assert(dt_soft.value>0.0);
                break;
            case 'o':
                dt_snp.value = atof(optarg);
                if(my_rank == 0) dt_snp.print(std::cout);
                assert(dt_snp.value>0.0);
                break;
            case 'r':
                r_out.value = atof(optarg);
                if(my_rank == 0) r_out.print(std::cout);
                assert(r_out.value>0.0);
                break;
            case 'b':
                n_bin.value = atoi(optarg);
                if(my_rank == 0) n_bin.print(std::cout);
                assert(n_bin.value>=0);
                break;
            case 'n':
                reading_flag=false;
                n_glb.value = atol(optarg);
                if(my_rank == 0) n_glb.print(std::cout);
                assert(n_glb.value>0);
                break;
            case 'G':
                n_group_limit.value = atoi(optarg);
                if(my_rank == 0) n_group_limit.print(std::cout);
                assert(n_group_limit.value>0);
                break;
            case 'L':
                n_leaf_limit.value = atoi(optarg);
                if(my_rank == 0) n_leaf_limit.print(std::cout);
                assert(n_leaf_limit.value>0);
                break;
            case 'S':
                n_smp_ave.value = atoi(optarg);
                if(my_rank == 0) n_smp_ave.print(std::cout);
                assert(n_smp_ave.value>0.0);
                break;
            case 'T':
                theta.value = atof(optarg);
                if(my_rank == 0) theta.print(std::cout);
                assert(theta.value>=0.0);
                break;
            case 'E':
                eta.value = atof(optarg);
                if(my_rank == 0) eta.print(std::cout);
                assert(eta.value>0.0);
                break;
            case 'f':
                fname_snp.value = optarg;
                if(my_rank == 0) fname_snp.print(std::cout);
                break;
            case 'p':
                fname_par.value = optarg;
                if(my_rank == 0) {
                    fname_par.print(std::cout);
                    FILE* fpar_in;
                    if( (fpar_in = fopen(fname_par.value.c_str(),"r")) == NULL) {
                        fprintf(stderr,"Error: Cannot open file %s.\n", fname_par.value.c_str());
                        abort();
                    }
                    input_par_store.readAscii(fpar_in);
                    fclose(fpar_in);
                }
#ifdef PARTICLE_SIMULATOR_MPI_PARALLEL        
                input_par_store.mpi_broadcast();
                PS::Comm::barrier();
#endif
                break;
            case 'h':
                if(my_rank == 0){
                    std::cout<<"Usage: nbody.out [option] [filename]"<<std::endl;
                    std::cout<<"       Option defaulted values are shown after ':'\n"<<std::endl;
                    std::cout<<"  -i: [I] "<<data_format<<std::endl;
                    std::cout<<"          File content:\n"
                             <<"            First line: \n"
                             <<"             1. File_ID: 0 for initialization, else for restarting\n"
                             <<"             2. N_particle \n"
                             <<"             3. N_offset: for naming special particle ID, should be > N_particle\n"
                             <<"             4. Time\n"
                             <<"             5. Tree time step (set 0 for input file)\n"
                             <<"             6. n_split (set 0 for input file)\n"
                             <<"            Following lines:\n"
                             <<"             1. mass\n"
                             <<"             2. position[3]\n"
                             <<"             5. velocity[3]\n"
                             <<"             8. r_search (0.0)\n"
                             <<"             9. mass_backup (0.0)\n"
                             <<"             10. ID (>0,unique)\n"
                             <<"             11. status (0)\n"
                             <<"             12. Acceleration[3] (0.0)\n"
                             <<"             15. Potential (0.0)\n"
                             <<"             16. N_neighbor (0)\n"
                             <<"             (*) show initialization values which should be used together with FILE_ID = 0"<<std::endl;
                    std::cout<<"  -a:     data output style (except snapshot) becomes appending, defaulted: replace"<<std::endl;
                    std::cout<<"  -t: [F] "<<time_end<<std::endl;
                    std::cout<<"  -s: [F] "<<dt_soft<<std::endl;
                    std::cout<<"  -o: [F] "<<dt_snp<<std::endl;
                    std::cout<<"        --dt-max-factor:   [F] "<<dt_limit_hard_factor<<std::endl;
                    std::cout<<"        --dt-min-hermite:  [I] "<<dt_min_hermite_index<<std::endl;
                    std::cout<<"  -r: [F] "<<r_out<<std::endl;
                    std::cout<<"        --r-ratio:         [F] "<<ratio_r_cut<<std::endl;
                    std::cout<<"        --r-bin:           [F] "<<r_bin<<std::endl;
                    std::cout<<"  -b: [I] "<<n_bin<<std::endl;
                    std::cout<<"  -n: [I] "<<n_glb<<std::endl;
                    std::cout<<"  -G: [I] "<<n_group_limit<<std::endl;
                    std::cout<<"  -L: [I] "<<n_leaf_limit<<std::endl;
                    std::cout<<"  -S: [I] "<<n_smp_ave<<std::endl;
                    std::cout<<"        --n-split:         [I] "<<n_split<<std::endl;
                    std::cout<<"  -T: [F] "<<theta<<std::endl;
                    std::cout<<"  -E: [F] "<<eta<<std::endl;
                    std::cout<<"        --search-factor:   [F] "<<search_factor<<std::endl;
                    std::cout<<"        --radius_factor:   [F] "<<radius_factor<<std::endl;
                    std::cout<<"        --energy-err-arc:  [F] "<<e_err_arc<<std::endl;
#ifdef HARD_CHECK_ENERGY
                    std::cout<<"        --energy-err-hard: [F] "<<e_err_hard<<std::endl;
#endif
#ifdef AR_SYM
                    std::cout<<"        --step-limit-arc:  [F] "<<step_limit_arc<<std::endl;
#endif
                    std::cout<<"        --slowdown-factor: [F] "<<sd_factor<<std::endl;
                    std::cout<<"        --soft-eps:        [F] "<<eps<<std::endl;
                    std::cout<<"  -f: [S] "<<fname_snp<<std::endl;
                    std::cout<<"  -p: [S] "<<fname_par<<std::endl;
                    std::cout<<"  -h(--help):               print help"<<std::endl;
                    std::cout<<"*** PS: r_in : transit function inner boundary radius\n"
                             <<"        r_out: transit function outer boundary radius\n"
                             <<"        sigma: half-mass radius velocity dispersion\n"
                             <<"        n_bin: number of primordial binaries\n"
                             <<"        <m>  : averaged mass"<<std::endl;
                }
                return -1;
            }
    

#ifdef PROFILE
        if(my_rank==0) {
            std::cout<<"----- Parallelization information -----\n";
            std::cout<<"MPI processors: "<<n_proc<<std::endl;
            std::cout<<"OMP threads:    "<<PS::Comm::getNumberOfThread()<<std::endl;
        }
    
        std::string rank_str;
        std::stringstream atmp;
        atmp<<my_rank;
        atmp>>rank_str;

        std::string fproname=fname_snp.value+".prof.rank."+rank_str;
        if(app_flag) fprofile.open(fproname.c_str(),std::ofstream::out|std::ofstream::app);
        else         fprofile.open(fproname.c_str(),std::ofstream::out);
#endif    

        // read dataset
        system_soft.initialize();
        system_soft.setAverageTargetNumberOfSampleParticlePerProcess(n_smp_ave.value);
        PS::S64 n_loc;
    
        if (reading_flag) {
            char* sinput=argv[argc-1];
            if(data_format.value==1||data_format.value==2||data_format.value==4)
                system_soft.readParticleAscii(sinput, file_header);
            else
                system_soft.readParticleBinary(sinput, file_header);
            stat.time = file_header.time;
            PS::Comm::broadcast(&stat.time, 1, 0);
            PS::Comm::broadcast(&file_header, 1, 0);
            n_glb.value = system_soft.getNumberOfParticleGlobal();
            n_loc = system_soft.getNumberOfParticleLocal();
            //      for(PS::S32 i=0; i<n_loc; i++) system_soft[i].id = i;
            if(my_rank==0)
                std::cout<<"Reading file "<<sinput<<std::endl
                         <<"N_tot = "<<n_glb.value<<"\nN_loc = "<<n_loc<<std::endl;
        }
        else {
            SetParticlePlummer(system_soft, n_glb.value, n_loc);
            file_header.nfile = 0;
            stat.time = file_header.time = 0.0;
            file_header.n_body = n_glb.value;
            //file_header.id_offset = n_glb.value;
            std::string fname = fname_snp.value+"."+std::to_string(file_header.nfile);
            if (data_format.value==1||data_format.value==3)
                system_soft.writeParticleAscii(fname.c_str(), file_header);
            else if(data_format.value==0||data_format.value==2)
                system_soft.writeParticleBinary(fname.c_str(), file_header);
        }

        PS::S64 id_offset = n_glb.value+1;
        
#ifdef MAIN_DEBUG
        for (int i=0; i<n_loc; i++) {
            assert(id_offset>system_soft[i].id);
        }
#endif

        // get parameters
        PS::F64 r_in, m_average, m_max, v_disp, v_max;
        GetInitPar(system_soft, r_in, r_out.value, r_bin.value, r_search_min, r_search_max.value, v_max, m_average, m_max, dt_soft.value, v_disp, search_factor.value, ratio_r_cut.value, n_bin.value, theta.value);

        //    EPISoft::r_out = r_out;
        EPISoft::eps   = eps.value;
        EPISoft::r_in  = r_in;
        EPISoft::r_out = r_out.value;
        Ptcl::search_factor = search_factor.value;
        Ptcl::r_search_min = r_search_min;
        Ptcl::mean_mass_inv = 1.0/m_average;
        Ptcl::r_group_crit_ratio = r_bin.value/r_in;
        //    const PS::F64 r_oi_inv = 1.0/(r_out - r_in);
        //    EPJSoft::r_search_min = r_out*search_factor;
        //    EPJSoft::m_average = m_average;
        this->dt_tree = dt_soft.value;
        this->dt_output = dt_snp.value;
        this->time_end = time_end.value;
        this->search_cluster_radius_factor = radius_factor.value;
        this->filename_output = fname_snp.value;
        this->output_data_format = data_format.value;

        // check restart
        bool restart_flag = file_header.nfile; // nfile = 0 is assumed as initial data file

#ifdef PARTICLE_SIMULATOR_MPI_PARALLEL
        PS::Comm::barrier();
#endif

        // file header update
        if(!restart_flag) file_header.n_body = system_soft.getNumberOfParticleGlobal();

        // open output files
        // status information output
        if(my_rank==0) {
            if(app_flag) fstatus.open((fname_snp.value+".status").c_str(),std::ofstream::out|std::ofstream::app);
            else         fstatus.open((fname_snp.value+".status").c_str(),std::ofstream::out);
            fstatus<<std::setprecision(WRITE_PRECISION);
#ifdef MAIN_DEBUG
            fout.open("nbody.dat");
#endif
        }

        if(!restart_flag&&my_rank==0&&app_flag==false)  {
            stat.printColumnTitle(fstatus,WRITE_WIDTH);
            fstatus<<std::endl;
        }

        // ID safety check
        if (!restart_flag) {
            for (PS::S32 i=0; i<n_loc; i++) {
                PS::S64 id = system_soft[i].id;
                if(id<=0) {
                    std::cerr<<"Error: for initial data, the id should always larger than zero. current index i = "<<i<<", id = "<<system_soft[i].id<<"!"<<std::endl;
                    abort();
                }

                // for binary, research depend on v_disp
                //if(id<=2*n_bin.value) system_soft[i].r_search = std::max(r_search_min*std::sqrt(system_soft[i].mass*Ptcl::mean_mass_inv),v_disp*dt_soft.value*search_factor.value);
                PS::F64 m_fac = system_soft[i].mass*Ptcl::mean_mass_inv;
                system_soft[i].changeover.setR(m_fac, r_in, r_out.value);
                if(id<=2*n_bin.value) system_soft[i].r_search = std::max(r_search_min,v_disp*dt_soft.value*search_factor.value + system_soft[i].changeover.getRout());
                else system_soft[i].calcRSearch(dt_soft.value);
            }
        }
    
        if(my_rank == 0) {
            std::cout<<"----- Parameter list: -----\n";
            std::cout<<" m_average    = "<<m_average      <<std::endl
                     <<" r_in         = "<<r_in           <<std::endl
                     <<" r_out        = "<<r_out.value    <<std::endl
                     <<" r_bin        = "<<r_bin.value    <<std::endl
                     <<" r_search_min = "<<r_search_min   <<std::endl
                     <<" vel_disp     = "<<v_disp         <<std::endl
                     <<" dt_soft      = "<<dt_soft.value  <<std::endl;
        }

        // save initial parameters
        if(my_rank==0) {
            std::cout<<"Save input parameters to file "<<fname_par.value<<std::endl;
            FILE* fpar_out;
            if( (fpar_out = fopen(fname_par.value.c_str(),"w")) == NULL) {
                fprintf(stderr,"Error: Cannot open file %s.\n", fname_par.value.c_str());
                abort();
            }
            input_par_store.writeAscii(fpar_out);
            fclose(fpar_out);
        }

        // system hard paramters
        hard_manager.setDtRange(dt_soft.value/dt_limit_hard_factor.value, dt_min_hermite_index.value);
        hard_manager.setEpsSq(eps.value);
        hard_manager.setG(1.0);
#ifdef HARD_CHECK_ENERGY
        hard_manager.energy_error_max = e_err_hard.value;
#else
        hard_manager.energy_error_max = NUMERIC_FLOAT_MAX;
#endif
        hard_manager.ap_manager.r_tidal_tensor = r_bin.value;
        hard_manager.ap_manager.r_in_base = r_in;
        hard_manager.ap_manager.r_out_base = r_out.value;
        hard_manager.ap_manager.id_offset = id_offset;
        hard_manager.ap_manager.setOrbitalParticleSplitN(n_split.value);
        //hard_manager.h4_manager.r_break_crit = r_bin.value;
        //hard_manager.h4_manager.r_neighbor_crit = r_search_min;
        hard_manager.h4_manager.step.eta_4th = eta.value;
        hard_manager.h4_manager.step.eta_2nd = 0.01*eta.value;
        hard_manager.h4_manager.step.calcAcc0OffsetSq(m_average, r_out.value);
        hard_manager.ar_manager.energy_error_relative_max = e_err_arc.value;
#ifdef AR_SYM
        hard_manager.ar_manager.step_count_max = step_limit_arc.value;
#endif
        // set symplectic order
        hard_manager.ar_manager.step.initialSymplecticCofficients(-6);
        hard_manager.ar_manager.slowdown_pert_ratio_ref = sd_factor.value;
        hard_manager.ar_manager.slowdown_timescale_max = dt_soft.value;
        hard_manager.ar_manager.slowdown_mass_ref = m_average;

        // check consistence of paramters
        hard_manager.checkParams();

        // dump paramters for restart
        if(my_rank==0) {
            std::string fhard_par = fname_par.value + ".hard";
            std::cout<<"Save hard_manager parameters to file "<<fhard_par<<std::endl;
            FILE* fpar_out;
            if( (fpar_out = fopen(fhard_par.c_str(),"w")) == NULL) {
                fprintf(stderr,"Error: Cannot open file %s.\n", fhard_par.c_str());
                abort();
            }
            hard_manager.writeBinary(fpar_out);
            fclose(fpar_out);
        }

        // initial hard class and parameters
        system_hard_one_cluster.manager = &hard_manager;
        system_hard_isolated.manager = &hard_manager;
        system_hard_connected.manager = &hard_manager;

#ifdef HARD_DUMP
        // initial hard_dump 
        const PS::S32 num_thread = PS::Comm::getNumberOfThread();
        hard_dump.initial(num_thread);
#endif

        // domain decomposition
        const PS::F32 coef_ema = 0.2;
        domain_decompose_weight=1.0;
        dinfo.initialize(coef_ema);
        dinfo.decomposeDomainAll(system_soft,domain_decompose_weight);

        if(pos_domain!=NULL) {
            pos_domain = new PS::F64ort[n_proc];
            for(PS::S32 i=0; i<n_proc; i++) pos_domain[i] = dinfo.getPosDomain(i);
        }

        // exchange particles
        system_soft.exchangeParticle(dinfo); 

        n_loc = system_soft.getNumberOfParticleLocal();

        // set local address
#pragma omp parallel for
        for(PS::S32 i=0; i<n_loc; i++){
            system_soft[i].rank_org = my_rank;
            system_soft[i].adr = i;
        }

        // tree for neighbor search
        tree_nb.initialize(n_glb.value, theta.value, n_leaf_limit.value, n_group_limit.value);

        // tree for force
        PS::S64 n_tree_init = n_glb.value + n_bin.value;
        tree_soft.initialize(n_tree_init, theta.value, n_leaf_limit.value, n_group_limit.value);

        // initial search cluster
        search_cluster.initialize();

        return 0;
    }


    //! integrate the system
    /*! @param[in] _time_break: additional breaking time to interupt the integration, in default the system integrate to time_end
     */
    PS::S32 evolveToTime(const PS::F64 _time_break=0.0) {

#ifdef PETAR_DEBUG
        PS::F64 time_drift = stat.time, time_kick = stat.time;
#endif
        // check time break
        PS::F64 time_break = _time_break==0.0? NUMERIC_FLOAT_MAX: _time_break;
        if (stat.time>=time_break) return 0;

        KickDriftStep dt_manager(dt_tree);
        PS::F64 dt_reduce_factor=1.0;
        bool first_step_flag = true;

        /// Main loop
        while(stat.time <= time_end){

#ifdef PROFILE
            PS::Comm::barrier();
            profile.tot.start();
#endif

            // >1. Tree for neighbor searching 
            /// get neighbor list to tree_nb
            treeNeighborSearch();

            // >2. search clusters
            /// gether clusters information to search_cluster, using tree_nb and velocity criterion (particles status/mass_bk)
            searchCluster();

            // >3. find group and create artificial particles
            /// find group and create artificial particles, using search_cluster, save to system_hard and system_soft (particle status/mass_bk updated)
            createGroup();

            // >4 tree soft force
            /// calculate tree force with linear cutoff, save to system_soft.acc
            treeSoftForce() ;

            // >5 correct change over
            /// correct system_soft.acc with changeover, using system_hard and system_soft particles
            treeForceCorrectChangeover();

#ifdef KDKDK_4TH
            // only do correction at middle step
            if (dt_manager.getCountContinue() == 1) GradientKick();
#endif

            bool interupt_flag = false;  // for interupt integration 
            bool output_flag = false;    // for output snapshot and information
            bool dt_mod_flag = false;    // for check whether tree time step need update
            bool changeover_flag = false; // for check whether changeover need update
            PS::F64 dt_kick, dt_drift;

            // for initial the system
            if(initial_step_flag) {
                initial_step_flag = false;
                first_step_flag = false;

                // correct force due to the change over update
                correctForceChangeOverUpdate();

                updateStatus(true);
                output(true);

                dt_kick = dt_manager.getDtStartContinue();
            }
            else if (first_step_flag) {
                first_step_flag = false;
                dt_kick = dt_manager.getDtStartContinue();
            }
            else {
                dt_kick = dt_manager.getDtKickContinue();

                // check whether tree time step need update only when one full step finish
                if (dt_manager.getCountContinue() == 0) {

                    // adjust tree step
                    dt_mod_flag = adjustDtTreeReduce(dt_reduce_factor, dt_tree, dt_manager.getStep());
                    if(dt_mod_flag) dt_kick = dt_manager.getDtEndContinue();

                    // output step, get last kick step
                    if( fmod(stat.time, dt_output) == 0.0) {
                        output_flag = true;
                        dt_kick = dt_manager.getDtEndContinue();
                    }

                    // check changeover change
                    if (system_hard_isolated.getNClusterChangeOverUpdate()>0) {
                        changeover_flag = true;
                        dt_kick = dt_manager.getDtEndContinue();
                    }

#ifdef PARTICLE_SIMULATOR_MPI_PARALLEL        
                    PS::S32 n_changeover_modify_local  = system_hard_connected.getNClusterChangeOverUpdate() + system_hard_isolated.getNClusterChangeOverUpdate();
                    PS::S32 n_changeover_modify_global = PS::Comm::getSum(n_changeover_modify_local);
                    if (n_changeover_modify_global>0) {
                        changeover_flag = true;
                        dt_kick = dt_manager.getDtEndContinue();
                    }
#endif

                    // check interuption
                    if (stat.time>=time_break) {
                        interupt_flag = true;
                        dt_kick = dt_manager.getDtEndContinue();
                    }
                    
                }
            }


            // >6. kick 
            kick(dt_kick);
#ifdef PETAR_DEBUG
            time_kick += dt_kick;
#endif

            // >7. write back data
            if(output_flag||interupt_flag) {
                // update global particle system due to kick
                system_hard_isolated.writeBackPtclForMultiCluster(system_soft, remove_list);
#ifdef PARTICLE_SIMULATOR_MPI_PARALLEL
                // update gloabl particle system and send receive remote particles
                search_cluster.writeAndSendBackPtcl(system_soft, system_hard_connected.getPtcl(), remove_list);
#endif
            }

            // output information
            if(output_flag) {
                // update status
                updateStatus(false);
                output(false);
            }

            // interupt
            if(interupt_flag) {
#ifdef CLUSTER_VELOCITY
                // update status and mass_bk to pcm data for search cluster after restart
                system_hard_one_cluster.resetParticleStatus(system_soft);
                system_hard_isolated.setParticleStatusToCMData(system_soft);
#ifdef PARTICLE_SIMULATOR_MPI_PARALLEL
                system_hard_connected.setParticleStatusToCMData(system_soft);
                search_cluster.writeAndSendBackPtcl(system_soft, system_hard_connected.getPtcl(), remove_list);
#endif                
#endif
                // correct force due to the change over update
                //correctForceChangeOverUpdate();
                // need remove artificial particles
                system_soft.setNumberOfParticleLocal(stat.n_real_loc);
#ifdef PETAR_DEBUG
                assert(time_kick==time_drift);
                assert(time_kick==stat.time);
#endif
                return 0;
            }

            // modify the tree step
            if(dt_mod_flag) {
                dt_manager.setStep(dt_tree/dt_reduce_factor);
                std::cout<<"Tree time step change, time = "<<stat.time
                         <<"  dt_soft = "<<dt_tree
                         <<"  reduce factor = "<<dt_reduce_factor
                         <<std::endl;
            }

            // second kick if dt_tree is changed or output is done
            if(dt_mod_flag||output_flag||changeover_flag) {
#ifdef PETAR_DEBUG
                assert(time_kick==time_drift);
                assert(time_kick==stat.time);
#endif
                //update new tree step if reduce factor is changed
                dt_kick = dt_manager.getDtStartContinue();

                correctForceChangeOverUpdate();

                kick(dt_kick);
#ifdef PETAR_DEBUG
                time_kick += dt_kick;
#endif
            }


            // >8. Hard integration 
            // get drift step
            dt_drift = dt_manager.getDtDriftContinue();

            drift(dt_drift);
            
            // determine the next tree time step
            PS::F64 dt_reduce_factor_org = 1.0;
            dt_reduce_factor_org = PS::Comm::getMaxValue(dt_reduce_factor_org);
            dt_reduce_factor = 1.0;
            while(dt_reduce_factor<dt_reduce_factor_org) dt_reduce_factor *=2.0;

            // remove artificial and unused particles
            removeParticles();

            // >9. Domain decomposition
            domainDecompose();
            // >10. exchange particles
            exchangeParticle();

            // advance stat.time and step count
            stat.time += dt_drift;
#ifdef PETAR_DEBUG
            time_drift += dt_drift;
#endif
            dt_manager.nextContinue();

            // profile
            calcProfile();

            n_loop++;
        }

        return 0;
    }

    void clear() {
#ifdef MAIN_DEBUG
        fout.close();
#endif
        if (pos_domain) {
            delete[] pos_domain;
            pos_domain=NULL;
        }
        
        if (init_flag) PS::Finalize();
        init_flag = false;
        initial_step_flag = true;
    }

    ~PeTar() { clear();}
};
