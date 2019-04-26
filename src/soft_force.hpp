#pragma once
#ifdef INTRINSIC_K
#include"phantomquad_for_p3t_k.hpp"
#endif
#ifdef INTRINSIC_X86
#include"phantomquad_for_p3t_x86.hpp"
#endif

////////////////////
/// FORCE FUNCTOR
struct CalcForceEpEpWithLinearCutoffNoSimd{
    void operator () (const EPISoft * ep_i,
                      const PS::S32 n_ip,
                      const EPJSoft * ep_j,
                      const PS::S32 n_jp,
                      ForceSoft * force){
        const PS::F64 eps2 = EPISoft::eps * EPISoft::eps;
        const PS::F64 r_out2 = EPISoft::r_out*EPISoft::r_out;
        //        const PS::F64 r_crit2 = EPJSoft::r_search * EPJSoft::r_search * SAFTY_FACTOR_FOR_SEARCH_SQ;
        // const PS::F64 r_out = EPISoft::r_out; 
        // const PS::F64 r_in = EPISoft::r_in;
        //std::cerr<<"r_out= "<<r_out<<" r_in= "<<r_in<<" eps2= "<<eps2<<" r_crit2= "<<r_crit2<<std::endl;
        for(PS::S32 i=0; i<n_ip; i++){
            const PS::F64vec xi = ep_i[i].pos;
            //PS::S64 id_i = ep_i[i].id;
            PS::F64vec ai = 0.0;
            PS::F64 poti = 0.0;
            PS::S32 n_ngb_i = 0;
            for(PS::S32 j=0; j<n_jp; j++){
                //if(id_i == ep_j[j].id){
                //    n_ngb_i++;
                //    continue;
                //}
                const PS::F64vec rij = xi - ep_j[j].pos;
                const PS::F64 r2 = rij * rij;
                const PS::F64 r2_eps = r2 + eps2;
                const PS::F64 r_search = std::max(ep_i[i].r_search,ep_j[j].r_search);
                if(r2 < r_search*r_search){
                    n_ngb_i++;
                }
                const PS::F64 r2_tmp = (r2_eps > r_out2) ? r2_eps : r_out2;
                const PS::F64 r_inv = 1.0/sqrt(r2_tmp);
                const PS::F64 m_r = ep_j[j].mass * r_inv;
                const PS::F64 m_r3 = m_r * r_inv * r_inv;
                ai -= m_r3 * rij;
                poti -= m_r;
            }
            //std::cerr<<"poti= "<<poti<<std::endl;
            force[i].acc += ai;
#ifdef KDKDK_4TH
            force[i].acorr = 0.0;
#endif
            force[i].pot += poti;
            force[i].n_ngb = n_ngb_i;
        }
    }
};

#ifdef KDKDK_4TH
struct CalcCorrectEpEpWithLinearCutoffNoSimd{
    void operator () (const EPISoft * ep_i,
                      const PS::S32 n_ip,
                      const EPJSoft * ep_j,
                      const PS::S32 n_jp,
                      ForceSoft * force){
        const PS::F64 eps2 = EPISoft::eps * EPISoft::eps;
        const PS::F64 r_out2 = EPISoft::r_out*EPISoft::r_out;

        for(PS::S32 i=0; i<n_ip; i++){
            PS::F64vec acorr = 0.0;
            const PS::F64vec posi = ep_i[i].pos;
            const PS::F64vec acci = ep_i[i].acc;
            for(PS::S32 j=0; j<n_jp; j++){
                const PS::F64vec dr = posi - ep_j[j].pos;
                const PS::F64vec da = acci - ep_j[j].acc; 
                const PS::F64 r2    = dr * dr + eps2;
                const PS::F64 drda  = dr * da;
                const PS::F64 r2_tmp = (r2 > r_out2) ? r2 : r_out2;
                const PS::F64 r_inv = 1.0/sqrt(r2_tmp);
                const PS::F64 r2_inv = r_inv*r_inv;
                const PS::F64 m_r = ep_j[j].mass * r_inv;
                const PS::F64 m_r3 = m_r * r2_inv;

                const PS::F64 alpha = 3.0 * drda * r2_inv;
                acorr -= m_r3 * (da - alpha * dr); 
            }
            //std::cerr<<"poti= "<<poti<<std::endl;
            force[i].acorr += 2.0 * acorr;
            force[i].acc = acci;
        }
    }
};
#endif

struct CalcForceEpSpMonoNoSimd {
    template<class Tsp>
    void operator () (const EPISoft * ep_i,
                      const PS::S32 n_ip,
                      const Tsp * sp_j,
                      const PS::S32 n_jp,
                      ForceSoft * force){
        const PS::F64 eps2 = EPISoft::eps * EPISoft::eps;
        for(PS::S32 i=0; i<n_ip; i++){
            PS::F64vec xi = ep_i[i].pos;
            PS::F64vec ai = 0.0;
            PS::F64 poti = 0.0;
            for(PS::S32 j=0; j<n_jp; j++){
                PS::F64vec rij = xi - sp_j[j].getPos();
                PS::F64 r3_inv = rij * rij + eps2;
                PS::F64 r_inv = 1.0/sqrt(r3_inv);
                r3_inv = r_inv * r_inv;
                r_inv *= sp_j[j].getCharge();
                r3_inv *= r_inv;
                ai -= r3_inv * rij;
                poti -= r_inv;
            }
            force[i].acc += ai;
            force[i].pot += poti;
        }
    }
};

struct CalcForceEpSpQuadNoSimd{
    template<class Tsp>
    void operator () (const EPISoft * ep_i,
                      const PS::S32 n_ip,
                      const Tsp * sp_j,
                      const PS::S32 n_jp,
                      ForceSoft * force){
        const PS::F64 eps2 = EPISoft::eps * EPISoft::eps;
//        assert(n_jp==0);
        for(PS::S32 ip=0; ip<n_ip; ip++){
            PS::F64vec xi = ep_i[ip].pos;
            PS::F64vec ai = 0.0;
            PS::F64 poti = 0.0;
            for(PS::S32 jp=0; jp<n_jp; jp++){
                PS::F64 mj = sp_j[jp].mass;
                PS::F64vec xj= sp_j[jp].pos;
                PS::F64vec rij= xi - xj;
                PS::F64 r2 = rij * rij + eps2;
                PS::F64mat qj = sp_j[jp].quad;
                PS::F64 tr = qj.getTrace();
                PS::F64vec qr( (qj.xx*rij.x + qj.xy*rij.y + qj.xz*rij.z),
                               (qj.yy*rij.y + qj.yz*rij.z + qj.xy*rij.x),
                               (qj.zz*rij.z + qj.xz*rij.x + qj.yz*rij.y) );
                PS::F64 qrr = qr * rij;
                PS::F64 r_inv = 1.0f/sqrt(r2);
                PS::F64 r2_inv = r_inv * r_inv;
                PS::F64 r3_inv = r2_inv * r_inv;
                PS::F64 r5_inv = r2_inv * r3_inv * 1.5;
                PS::F64 qrr_r5 = r5_inv * qrr;
                PS::F64 qrr_r7 = r2_inv * qrr_r5;
                PS::F64 A = mj*r3_inv - tr*r5_inv + 5*qrr_r7;
                PS::F64 B = -2.0*r5_inv;
                ai -= A*rij + B*qr;
                poti -= mj*r_inv - 0.5*tr*r3_inv + qrr_r5;
            }
            force[ip].acc += ai;
            force[ip].pot += poti;
        }
    }
};

#ifdef USE_SIMD
struct CalcForceEpEpWithLinearCutoffSimd{
    void operator () (const EPISoft * ep_i,
                      const PS::S32 n_ip,
                      const EPJSoft * ep_j,
                      const PS::S32 n_jp,
                      ForceSoft * force){
        const PS::F64 eps2 = EPISoft::eps * EPISoft::eps;
        PS::S32 ep_j_list[n_jp], n_jp_local=0;
        for (PS::S32 i=0; i<n_jp; i++){
            if(ep_j[i].mass>0) ep_j_list[n_jp_local++] = i;
        }
        //std::cerr<<"n_jp="<<n_jp<<" reduced n_jp="<<n_jp_local<<std::endl;
//        const PS::F64 r_crit2 = EPJSoft::r_search * EPJSoft::r_search;
    #ifdef __HPC_ACE__
        PhantomGrapeQuad pg;
    #else
        #if defined(CALC_EP_64bit) || defined(CALC_EP_MIX)
        static __thread PhantomGrapeQuad64Bit pg;
        #else
        static __thread PhantomGrapeQuad pg;
        #endif
    #endif
        if(n_ip > pg.NIMAX || n_jp > pg.NJMAX){
            std::cout<<"ni= "<<n_ip<<" NIMAX= "<<pg.NIMAX<<" nj= "<<n_jp<<" NJMAX= "<<pg.NJMAX<<std::endl;
        }
        assert(n_ip<=pg.NIMAX);
        assert(n_jp<=pg.NJMAX);
        pg.set_eps2(eps2);
        pg.set_r_crit2(EPISoft::r_out*EPISoft::r_out);
        //pg.set_cutoff(EPISoft::r_out, EPISoft::r_in);
        for(PS::S32 i=0; i<n_ip; i++){
            const PS::F64vec pos_i = ep_i[i].getPos();
            pg.set_xi_one(i, pos_i.x, pos_i.y, pos_i.z, ep_i[i].r_search);
        }
        PS::S32 loop_max = (n_jp_local-1) / PhantomGrapeQuad::NJMAX + 1;
        for(PS::S32 loop=0; loop<loop_max; loop++){
            const PS::S32 ih = PhantomGrapeQuad::NJMAX*loop;
            const PS::S32 n_jp_tmp = ( (n_jp_local - ih) < PhantomGrapeQuad::NJMAX) ? (n_jp_local - ih) : PhantomGrapeQuad::NJMAX;
            const PS::S32 it =ih + n_jp_tmp;
            PS::S32 i_tmp = 0;
            for(PS::S32 i=ih; i<it; i++, i_tmp++){
                const PS::S32 ij = ep_j_list[i];
                const PS::F64 m_j = ep_j[ij].getCharge();
                const PS::F64vec pos_j = ep_j[ij].getPos();
                pg.set_epj_one(i_tmp, pos_j.x, pos_j.y, pos_j.z, m_j, ep_j[ij].r_search);

            }
            pg.run_epj_for_p3t_with_linear_cutoff(n_ip, n_jp_tmp);
            for(PS::S32 i=0; i<n_ip; i++){
                PS::F64 * p = &(force[i].pot);
                PS::F64 * a = (PS::F64 * )(&force[i].acc[0]);
                PS::F64 n_ngb = 0;
                pg.accum_accp_one(i, a[0], a[1], a[2], *p, n_ngb);
                force[i].n_ngb += (PS::S32)(n_ngb*1.00001);
            }
        }
    }
};

struct CalcForceEpSpMonoSimd{
    template<class Tsp>
    void operator () (const EPISoft * ep_i,
                      const PS::S32 n_ip,
                      //const PS::SPJMonopoleScatter * sp_j,
                      const Tsp * sp_j,
                      const PS::S32 n_jp,
                      ForceSoft * force){
        const PS::F64 eps2 = EPISoft::eps * EPISoft::eps;
#ifdef __HPC_ACE__
        PhantomGrapeQuad pg;
#else
    #if defined(CALC_EP_64bit)
        static __thread PhantomGrapeQuad64Bit pg;
    #else
        static __thread PhantomGrapeQuad pg;
    #endif
#endif
        assert(n_ip<=pg.NIMAX);
        assert(n_jp<=pg.NJMAX);
        pg.set_eps2(eps2);
        for(PS::S32 i=0; i<n_ip; i++){
            const PS::F64vec pos_i = ep_i[i].getPos();
            pg.set_xi_one(i, pos_i.x, pos_i.y, pos_i.z, 0.0);
        }
        PS::S32 loop_max = (n_jp-1) / PhantomGrapeQuad::NJMAX + 1;
        for(PS::S32 loop=0; loop<loop_max; loop++){
            const PS::S32 ih = PhantomGrapeQuad::NJMAX*loop;
            const PS::S32 n_jp_tmp = ( (n_jp - ih) < PhantomGrapeQuad::NJMAX) ? (n_jp - ih) : PhantomGrapeQuad::NJMAX;
            const PS::S32 it = ih + n_jp_tmp;
            PS::S32 i_tmp = 0;
            for(PS::S32 i=ih; i<it; i++, i_tmp++){
                const PS::F64 m_j = sp_j[i].getCharge();
                const PS::F64vec pos_j = sp_j[i].getPos();
                pg.set_epj_one(i_tmp, pos_j.x, pos_j.y, pos_j.z, m_j, 0.0);
            }
            pg.run_epj(n_ip, n_jp_tmp);
            for(PS::S32 i=0; i<n_ip; i++){
                PS::F64 * p = &(force[i].pot);
                PS::F64 * a = (PS::F64 * )(&force[i].acc[0]);
                pg.accum_accp_one(i, a[0], a[1], a[2], *p);
            }
        }
    }
};

struct CalcForceEpSpQuadSimd{
    template<class Tsp>
    void operator () (const EPISoft * ep_i,
                      const PS::S32 n_ip,
                      const Tsp * sp_j,
                      const PS::S32 n_jp,
                      ForceSoft * force){
        const PS::F64 eps2 = EPISoft::eps * EPISoft::eps;
    #ifdef __HPC_ACE__
        PhantomGrapeQuad pg;
    #else
        #if defined(CALC_EP_64bit)
        static __thread PhantomGrapeQuad64Bit pg;
        #else
        static __thread PhantomGrapeQuad pg;
        #endif
    #endif
        assert(n_ip<=pg.NIMAX);
        assert(n_jp<=pg.NJMAX);
        pg.set_eps2(eps2);
        for(PS::S32 i=0; i<n_ip; i++){
            const PS::F64vec pos_i = ep_i[i].getPos();
            pg.set_xi_one(i, pos_i.x, pos_i.y, pos_i.z, 0.0);
        }
        PS::S32 loop_max = (n_jp-1) / PhantomGrapeQuad::NJMAX + 1;
        for(PS::S32 loop=0; loop<loop_max; loop++){
            const PS::S32 ih = PhantomGrapeQuad::NJMAX*loop;
            const PS::S32 n_jp_tmp = ( (n_jp - ih) < PhantomGrapeQuad::NJMAX) ? (n_jp - ih) : PhantomGrapeQuad::NJMAX;
            const PS::S32 it = ih + n_jp_tmp;
            PS::S32 i_tmp = 0;
            for(PS::S32 i=ih; i<it; i++, i_tmp++){
                const PS::F64 m_j = sp_j[i].getCharge();
                const PS::F64vec pos_j = sp_j[i].getPos();
                const PS::F64mat q = sp_j[i].quad;
                pg.set_spj_one(i, pos_j.x, pos_j.y, pos_j.z, m_j,
                               q.xx, q.yy, q.zz, q.xy, q.yz, q.xz);
            }
            pg.run_spj(n_ip, n_jp_tmp);
            for(PS::S32 i=0; i<n_ip; i++){
                PS::F64 * p = &(force[i].pot);
                PS::F64 * a = (PS::F64 * )(&force[i].acc[0]);
                pg.accum_accp_one(i, a[0], a[1], a[2], *p);
            }
        }
    }
};
#endif