#pragma once
#include "Common/binary_tree.h"

//! stability checker for binary tree
template <class Tptcl>
class Stability {
public:
    typedef COMM::BinaryTree<Tptcl,COMM::Binary> BinTree;
    typedef COMM::Binary            Bin;
    PS::F64 t_crit;                             // period criterion to determine stability
    PS::ReallocatableArray<BinTree*> stable_binary_tree;  // to store the address of sub-branch of stable systems from a binary tree

    Stability(): t_crit(-1.0), stable_binary_tree() {}

    void clear() {
        t_crit=-1.0;
        stable_binary_tree.clear();
    }

    //! Three-body stability function 
    /* Use Myllaeri et al. (2018, MNRAS, 476, 830) stability criterion to check whether the system is stable.
       @param[in] _bin_in: inner binary
       @param[in] _bin_out: outer binary (m1 is the inner c.m. mass)
       @param[in] _incline: inclination angle between inner and outer orbit (radians)
       @param[in] _t_crit: time interval for stable check
       @param[in] _is_cm_in_first: true: _bin_out m1 is the inner c.m. mass; otherwise m2
       \return stability factor <1 stable; >1 unstable
    */
    static PS::F64 stable3body(const Bin& _bin_in,
                               const Bin& _bin_out,
                               const PS::F64 _incline,
                               const PS::F64 _t_crit,
                               const bool is_cm_in_first) {

        PS::F64 fac = 1.0 - 2.0*_bin_in.ecc/3.0 * (1.0 - 0.5*_bin_in.ecc*_bin_in.ecc) 
            - 0.3*std::cos(_incline)*(1.0 - 0.5*_bin_in.ecc + 2.0*std::cos(_incline)*(1.0 - 2.5*std::pow(_bin_in.ecc,1.5) - std::cos(_incline)));

        PS::F64 mout, min;
        if (is_cm_in_first) {
            min  = _bin_out.m1;
            mout = _bin_out.m2;
        }
        else {
            min  = _bin_out.m2;
            mout = _bin_out.m1;
        }
        PS::F64 g = std::sqrt(std::max(_bin_in.m1,_bin_in.m2) /min)*(1.0 + mout/min);
    
        //Adopt at least 10,000 outer orbits for random walk time-scale.
        PS::F64 q = 1.52*std::pow(std::sqrt(_t_crit/_bin_out.period)/(1.0 - _bin_out.ecc),1.0/6.0)*std::pow(fac*g,1.0/3.0);

        PS::F64 peri_out = _bin_out.semi * (_bin_out.ecc + 1.0);
        PS::F64 rp = peri_out/_bin_in.semi;
    
        PS::F64 stab = q/rp;
        return stab;
    }


    //! check two-body stability
    /* return       case
       Unstable:    false   hyperbolic orbit
       stable:      true    apo-center < _r_crit && period < _t_crit 
                    false   others
        
       @param[in,out] _bin: binary information
       @param[in] _r_crit: distance criterion to select stable binary
       @param[in] _t_crit: period criterion to select stable binary
    */
    static bool stable2check(const Bin& _bin, const PS::F64 _r_crit, const PS::F64 _t_crit) {

#ifdef STABLE_CHECK_DEBUG_PRINT
        std::cerr<<"STAB2: semi="<<_bin.semi
                 <<" ecc="<<_bin.ecc
                 <<" m1="<<_bin.m1
                 <<" m2="<<_bin.m2
                 <<" period="<<_bin.period
                 <<" apo="<<_bin.semi*(1.0+_bin.ecc)
                 <<" pec="<<_bin.semi*(1.0-_bin.ecc)
                 <<std::endl;
#endif
        // hyperbolic case
        PS::F64 semi = _bin.semi;
        if(semi<0) {
#ifdef STABLE_CHECK_DEBUG_PRINT
            std::cerr<<"STAB2 reject: Hyperbolic"<<std::endl;
#endif        
            return false;
        }

        // binary case
        PS::F64 apo = _bin.semi*(1.0+_bin.ecc);

        if (apo<_r_crit&&_bin.period<_t_crit)  {
#ifdef STABLE_CHECK_DEBUG_PRINT
            std::cerr<<"STAB2 accept: Stable"<<std::endl;
#endif 
            return true;
        }
        else {
#ifdef STABLE_CHECK_DEBUG_PRINT
            std::cerr<<"STAB2 reject: Stable but too large orbit, apo: "<<apo<<" ecc: "<<_bin.ecc<<" r_crit: "<<_r_crit
                     <<" period: "<<_bin.period<<" t_crit: "<<_t_crit<<std::endl;
#endif 
            return false;
        }
    }

    //! Three-body (B-S) stability check
    /* return              case
       false              hyperbolic outer orbit
       false              apo-center outer > r_crit || period outer > t_crit
       false              stab3 >1
       true               stab3 <1 
       @param[in] _bin: inner orbit parameter
       @param[in] _bout: outer orbit parameter
       @param[in] _r_crit: distance criterion to select stable binary
       @param[in] _t_crit: period criterion to select stable binary
       @param[in] _is_cm_in_first: true: _bin_out m1 is the inner c.m. mass; otherwise m2
    */
    static bool stable3check(const Bin& _bin,
                             const Bin& _bout,
                             const PS::F64 _r_crit,
                             const PS::F64 _t_crit,
                             const PS::F64 _is_cm_in_first) {
#ifdef STABLE_CHECK_DEBUG_PRINT
        std::cerr<<"STAB3 bout semi="<<_bout.semi
                 <<" ecc="<<_bout.ecc
                 <<" m1="<<_bout.m1
                 <<" m2="<<_bout.m2
                 <<" period="<<_bout.period
                 <<" apo="<<_bout.semi*(1.0+_bout.ecc)
                 <<" pec="<<_bout.semi*(1.0-_bout.ecc)
                 <<" bin semi="<<_bin.semi
                 <<" ecc="<<_bin.ecc
                 <<" m1="<<_bin.m1
                 <<" m2="<<_bin.m2
                 <<" period="<<_bin.period
                 <<" apo="<<_bin.semi*(1.0+_bin.ecc)
                 <<" pec="<<_bin.semi*(1.0-_bin.ecc)
                 <<std::endl;
#endif
        // hyperbolic okuter orbit
        if(_bout.semi<0) {
#ifdef STABLE_CHECK_DEBUG_PRINT
            std::cerr<<"STAB3 reject: Unstable, Outer body hyperbolic, semi_out: "<<_bout.semi<<std::endl;
#endif
            return false;
        }
        PS::F64 apo_out=_bout.semi*(1.0+_bout.ecc);

        // too large orbit
        if(apo_out>_r_crit) {
#ifdef STABLE_CHECK_DEBUG_PRINT
            std::cerr<<"STAB3 reject: Too large outer orbit, apo_out: "<<apo_out<<" r_crit: "<<_r_crit<<std::endl;
#endif
            return false;
        } 

        // too large period
        if(_bout.period>_t_crit) {
#ifdef STABLE_CHECK_DEBUG_PRINT
            std::cerr<<"STAB3 reject: Too large outer period, period_out: "<<_bout.period<<" t_crit: "<<_t_crit<<std::endl;
#endif
            return false;
        }
        
        // stability check
        // inclination between inner and outer orbit
        PS::F64 incline=std::acos(std::min(1.0, _bout.am*_bin.am/std::sqrt((_bout.am*_bout.am)*(_bin.am*_bin.am))));
        PS::F64 stab3 = stable3body(_bin, _bout, incline, _t_crit, _is_cm_in_first);
        if(stab3>1) {
            // Unstable case
#ifdef STABLE_CHECK_DEBUG_PRINT
            std::cerr<<"STAB3 reject: Unstable, stab3: "<<stab3<<std::endl;
#endif
            return false;
        }
        else {
            // stable case
#ifdef STABLE_CHECK_DEBUG_PRINT
            std::cerr<<"STAB3 accept: Stable, stab3: "<<stab3<<" period_in: "<<_bin.period<<" period_out: "<<_bout.period<<std::endl;
#endif
            return true;
        }
    }

    //! Four-body (B-B) stability check
    /*             return     tstep   stable_factor      case
                   Unstable:  false     -1         -1          hyperbolic outer orbit
                   false     -1         -1          apo-center outer > _r_crit
                   false     -1         -1          period outer > 0.25 * dt_tree
                   true      inner      -stab3_max  stab3_1 >0.8 || stab3_2 > 0.8 & apo_out <= r_out
                   false     -1         -1                                        & apo_out >  r_out
                   stable:     true      inner      -1          stab3_1 <=0.8 & stab3_2 <=0.8 & unpert & outer period > 1/8 dt_tree
                   false     -1         -1                                    --  & acceleration ratio(out/in) <1e-6 and outer period >1e-4 dt_tree
                   true      inner      fpert/(m_out/apo_out^2)               --  & other cases
                   @param[in] _bout: outer orbit parameter
                   @param[in] _bin1: first inner orbit parameter
                   @param[in] _bin2: second inner orbit parameter
                   @param[in] _r_crit: distance criterion to select stable binary
                   @param[in] _t_crit: period criterion to select stable binary
    */
    static bool stable4check(const Bin& _bin1,
                             const Bin& _bin2,
                             const Bin& _bout,
                             const PS::F64 _r_crit,
                             const PS::F64 _t_crit) {
#ifdef STABLE_CHECK_DEBUG_PRINT
        std::cerr<<"STAB4 bout semi="<<_bout.semi
                 <<" ecc="<<_bout.ecc
                 <<" m1="<<_bout.m1
                 <<" m2="<<_bout.m2
                 <<" period="<<_bout.period
                 <<" apo="<<_bout.semi*(1.0+_bout.ecc)
                 <<" pec="<<_bout.semi*(1.0-_bout.ecc)
                 <<" bin1 semi="<<_bin1.semi
                 <<" ecc="<<_bin1.ecc
                 <<" m1="<<_bin1.m1
                 <<" m2="<<_bin1.m2
                 <<" period="<<_bin1.period
                 <<" apo="<<_bin1.semi*(1.0+_bin1.ecc)
                 <<" pec="<<_bin1.semi*(1.0-_bin1.ecc)
                 <<" bin2 semi="<<_bin2.semi
                 <<" ecc="<<_bin2.ecc
                 <<" m1="<<_bin2.m1
                 <<" m2="<<_bin2.m2
                 <<" period="<<_bin2.period
                 <<" apo="<<_bin2.semi*(1.0+_bin2.ecc)
                 <<" pec="<<_bin2.semi*(1.0-_bin2.ecc)
                 <<std::endl;
#endif
        // hyperbolic outer orbit
        if(_bout.semi<0) {
#ifdef STABLE_CHECK_DEBUG_PRINT
            std::cerr<<"STAB4 reject: Unstable, Outer body hyperbolic, semi_out: "<<_bout.semi<<std::endl;
#endif
            return false;
        }

        PS::F64 apo_out=_bout.semi*(1.0+_bout.ecc);
        //PS::F64 pec_out=_bout.semi*(1.0-_bout.ecc);
        //PS::F64 apo_in1=_bin1.semi*(1.0+_bin1.ecc);
        //PS::F64 apo_in2=_bin2.semi*(1.0+_bin2.ecc);

    
        // too large orbit
        if(apo_out>_r_crit) {
#ifdef STABLE_CHECK_DEBUG_PRINT
            std::cerr<<"STAB4 reject: Unstable, Too large outer orbit, apo_out: "<<apo_out<<" r_crit: "<<_r_crit<<std::endl;
#endif
            return false;
        } 

        // too large period
        if(_bout.period>_t_crit) {
#ifdef STABLE_CHECK_DEBUG_PRINT
            std::cerr<<"STAB4 reject: Too large outer period, period_out: "<<_bout.period<<" t_crit: "<<_t_crit<<std::endl;
#endif
            return false;
        }
    
        // stability check
        // inclination between inner and outer orbit
        PS::F64 incl1=std::acos(std::min(1.0, _bout.am*_bin1.am/std::sqrt((_bout.am*_bout.am)*(_bin1.am*_bin1.am))));
        PS::F64 incl2=std::acos(std::min(1.0, _bout.am*_bin2.am/std::sqrt((_bout.am*_bout.am)*(_bin2.am*_bin2.am))));
        PS::F64 stab3_1 = stable3body(_bin1, _bout, incl1, _t_crit, true);
        PS::F64 stab3_2 = stable3body(_bin2, _bout, incl2, _t_crit, false);
        if(stab3_1>0.8||stab3_2>0.8) {
            // Unstable case
#ifdef STABLE_CHECK_DEBUG_PRINT
            std::cerr<<"STAB4 reject: Unstable, stab3_1: "<<stab3_1<<" stab3_2: "<<stab3_2<<std::endl;
#endif
            return false;
        }
        else {
            // stable case
#ifdef STABLE_CHECK_DEBUG_PRINT
            std::cerr<<"STAB4 accept: Stable,  stab3_1: "<<stab3_1<<" stab3_2: "<<stab3_2<<std::endl;
#endif
            return true;
        }
    }

    //! Closed system check iteration function for BinaryTree
    /*! check whether the orbit is closed system with apo-center distance less than r_group
       @param[in,out] _stab: stability class
       @param[in] _stab_res1/2: results from iteration of two branches. -1: branch is one particle; 0: unstable branch; 1: stable branch
       @param[in]  _bins: binary tree data 
     */
    static PS::S32 closedSystemCheckIter(Stability& _stab, const PS::S32& _stab_res1, const PS::S32& _stab_res2, BinTree& _bin) {
        // unstable case
        if (_stab_res1==0||_stab_res2==0) {
            // if any branch is stable, save data
            if (_stab_res1==1)  _stab.stable_binary_tree.push_back((BinTree*)_bin.getLeftMember());
            if (_stab_res2==1)  _stab.stable_binary_tree.push_back((BinTree*)_bin.getRightMember());
            return 0;
        }

        PS::F64 apo = _bin.semi*(1+_bin.ecc);
        bool stab_flag = (apo>0.0 && apo<=_bin.getRGroup());
        // B-B system
        if (_stab_res1==1&&_stab_res2==1) {
            if (stab_flag) return 1;
            else {
                _stab.stable_binary_tree.push_back((BinTree*)_bin.getLeftMember());
                _stab.stable_binary_tree.push_back((BinTree*)_bin.getRightMember());
                return 0;
            }
        }
        // triple case
        if (_stab_res1==-1&&_stab_res2==1) {
            if (stab_flag) return 1;
            else {
                _stab.stable_binary_tree.push_back((BinTree*)_bin.getRightMember());
                return 0;
            }
        }
        if (_stab_res2==-1&&_stab_res1==1) {
            if (stab_flag) return 1;
            else {
                _stab.stable_binary_tree.push_back((BinTree*)_bin.getLeftMember());
                return 0;
            }
        }
        // binary case
#ifdef STABLE_CHECK_DEBUG
        assert(_stab_res1==-1&&_stab_res2==-1);
#endif
        if (stab_flag) return 1;
        else return 0;
    }

    //! Check binary tree and collect closed systems subtree root
    /*! Check the orbit of each binary and if apo-center distance is positive and < bin.getRGroup(), selected as candidates.
        subtrees are stored in stable_binary_tree.
       @param[in]  _bins: binary tree data 
    */
    void findClosedTree(BinTree& _bin) {
        stable_binary_tree.resizeNoInitialize(0);
        bool is_stable= _bin.processTreeIter(*this, -1, -1, closedSystemCheckIter);
        if (is_stable) stable_binary_tree.push_back(&_bin);
    }

    //! Stability check iteraction function for BinaryTree
    /*! Check stability of binary, triple and B-B for given binarytree, for sub branch of stable systems, save to stable_binary_tree
       @param[in,out] _stab: stability class
       @param[in] _stab_res1/2: results from iteration of two branches. -1: branch is one particle; 0: unstable branch; 1: stable branch
       @param[in]  _bins: binary tree data 
    */
    static PS::S32 stabilityCheckIter(Stability& _stab, const PS::S32& _stab_res1, const PS::S32& _stab_res2, BinTree& _bin) {
        // B-B system
        if (_stab_res1==1&&_stab_res2==1) {
            bool stab4=stable4check(*(BinTree*)_bin.getLeftMember(), *(BinTree*)_bin.getRightMember(), _bin, _bin.getRGroup(), _stab.t_crit);
            if (stab4) return 1;
            else {
                _stab.stable_binary_tree.push_back((BinTree*)_bin.getLeftMember());
                _stab.stable_binary_tree.push_back((BinTree*)_bin.getRightMember());
                return 0;
            }
        }
        // unstable case
        if (_stab_res1==0||_stab_res2==0) {
            // if any branch is stable, save data
            if (_stab_res1==1)  _stab.stable_binary_tree.push_back((BinTree*)_bin.getLeftMember());
            if (_stab_res2==1)  _stab.stable_binary_tree.push_back((BinTree*)_bin.getRightMember());
            return 0;
        }
        // triple case
        if (_stab_res1==-1&&_stab_res2==1) {
            bool stab3=stable3check(*(BinTree*)_bin.getRightMember(), _bin, _bin.getRGroup(), _stab.t_crit, false);
            if (stab3) return 1;
            else {
                _stab.stable_binary_tree.push_back((BinTree*)_bin.getRightMember());
                return 0;
            }
        }
        if (_stab_res2==-1&&_stab_res1==1) {
            bool stab3=stable3check(*(BinTree*)_bin.getLeftMember(), _bin, _bin.getRGroup(), _stab.t_crit, true);
            if (stab3) return 1;
            else {
                _stab.stable_binary_tree.push_back((BinTree*)_bin.getLeftMember());
                return 0;
            }
        }
        // binary case
#ifdef STABLE_CHECK_DEBUG
        assert(_stab_res1==-1&&_stab_res2==-1);
#endif
        bool stab2 = stable2check(_bin, _bin.getRGroup(), _stab.t_crit);
        if (stab2) return 1;
        else return 0;
    }

    //! Check binary tree and collect stable subtree
    /*!
       @param[in]  _bins: binary tree data 
    */
    void findStableTree(BinTree& _bin) {
        stable_binary_tree.resizeNoInitialize(0);
        bool is_stable= _bin.processTreeIter(*this, -1, -1, stabilityCheckIter);
        if (is_stable) stable_binary_tree.push_back(&_bin);
    }
    
};
