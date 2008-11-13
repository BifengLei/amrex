//
// $Id: FabSet.cpp,v 1.52 2008-11-13 16:58:28 lijewski Exp $
//
#include <winstd.H>

#include <map>

#include <FabSet.H>
#include <ParallelDescriptor.H>

FabSetIter::FabSetIter (const FabSet& fabset)
    :
    MFIter(fabset)
{}

FabSetIter::~FabSetIter () {}

FabSetCopyDescriptor::FabSetCopyDescriptor ()
    :
    MultiFabCopyDescriptor() {}

FabSetCopyDescriptor::~FabSetCopyDescriptor () {}

FabSetId
FabSetCopyDescriptor::RegisterFabSet (FabSet* fabset)
{
    return RegisterMultiFab(fabset);
}

FabSet::FabSet () {}

FabSet::~FabSet () {}

FabSet::FabSet (const BoxArray& grids, int ncomp)
    :
    MultiFab(grids,ncomp,0,Fab_allocate)
{}

void
FabSet::define (const BoxArray& grids, int ncomp)
{
    MultiFab* tmp = this;

    tmp->define(grids, ncomp, 0, Fab_allocate);
}

const FabSet&
FabSet::copyTo (FArrayBox& dest) const
{
    copy(dest);
    return *this;
}

const FabSet&
FabSet::copyTo (FArrayBox& dest,
                int        src_comp,
                int        dest_comp,
                int        num_comp) const
{
    copy(dest,src_comp,dest_comp,num_comp);
    return *this;
}

const FabSet&
FabSet::copyTo (FArrayBox& dest,
                const Box& subbox,
                int        src_comp,
                int        dest_comp,
                int        num_comp) const
{
    copy(dest,subbox,src_comp,dest_comp,num_comp);
    return *this;
}

void
FabSet::copyTo (MultiFab& dest) const
{
    dest.copy(*this);
}

FabSet&
FabSet::copyFrom (const FabSet& src)
{
    copy(src);
    return *this;
}

FabSet&
FabSet::copyFrom (const FabSet& src,
                  int           src_comp,
                  int           dest_comp,
                  int           num_comp)
{
    copy(src,src_comp,dest_comp,num_comp);
    return *this;
}

//
// The following are different from MultiFab only in the return value
//

FabSet&
FabSet::plus (Real v,
              int  comp,
              int  num_comp)
{
    MultiFab* tmp = this;
    tmp->plus(v, comp, num_comp);
    return *this;
}

FabSet&
FabSet::plus (Real       v,
              const Box& subreg,
              int        comp,
              int        num_comp)
{
    MultiFab* tmp = this;
    tmp->plus(v, subreg, comp, num_comp);
    return *this;
}

FabSet&
FabSet::mult (Real v,
              int  comp,
              int  num_comp)
{
    MultiFab* tmp = this;
    tmp->mult(v, comp, num_comp);
    return *this;
}

FabSet&
FabSet::mult (Real       v,
              const Box& subreg,
              int        comp,
              int        num_comp)
{
    MultiFab* tmp = this;
    tmp->mult(v, subreg, comp, num_comp);
    return *this;
}


FabSet&
FabSet::copyFrom (const FArrayBox& src)
{
    for (FabSetIter fsi(*this); fsi.isValid(); ++fsi)
    {
        get(fsi).copy(src);
    }
    return *this;
}

FabSet&
FabSet::copyFrom (const FArrayBox& src,
                  int              src_comp,
                  int              dest_comp,
                  int              num_comp)
{
    for (FabSetIter fsi(*this); fsi.isValid(); ++fsi)
    {
        get(fsi).copy(src,src_comp,dest_comp,num_comp);
    }
    return *this;
}

FabSet&
FabSet::copyFrom (const FArrayBox& src,
                  const Box&       subbox,
                  int              src_comp,
                  int              dest_comp,
                  int              num_comp)
{
    BL_ASSERT(src.box().contains(subbox));

    for (FabSetIter fsi(*this); fsi.isValid(); ++fsi)
    {
        Box dbox = get(fsi).box() & subbox;

        if (dbox.ok())
        {
            get(fsi).copy(src,dbox,src_comp,dbox,dest_comp,num_comp);
        }
    }

    return *this;
}

//
// Used in caching CollectData() stuff for copyFrom() and plusFrom().
//
struct FSRec
{
    FSRec ();

    FSRec (const BoxArray&            src,
           const BoxArray&            dst,
           const DistributionMapping& srcdm,
           const DistributionMapping& dstdm,
           int                        ngrow);

    FSRec (const FSRec& rhs);

    ~FSRec ();

    bool operator== (const FSRec& rhs) const;
    bool operator!= (const FSRec& rhs) const;

    std::vector<Box>    m_box;
    std::vector<int>    m_mfidx;
    std::vector<int>    m_fsidx;
    Array<int>          m_snds;
    CommDataCache       m_commdata;
    BoxArray            m_src;
    BoxArray            m_dst;
    DistributionMapping m_srcdm;
    DistributionMapping m_dstdm;
    int                 m_ngrow;
    bool                m_reused;
};

FSRec::FSRec ()
    :
    m_ngrow(-1),
    m_reused(false)
{}

FSRec::FSRec (const BoxArray&            src,
              const BoxArray&            dst,
              const DistributionMapping& srcdm,
              const DistributionMapping& dstdm,
              int                        ngrow)
    :
    m_src(src),
    m_dst(dst),
    m_srcdm(srcdm),
    m_dstdm(dstdm),
    m_ngrow(ngrow),
    m_reused(false)
{
    BL_ASSERT(ngrow >= 0);
}

FSRec::FSRec (const FSRec& rhs)
    :
    m_box(rhs.m_box),
    m_mfidx(rhs.m_mfidx),
    m_fsidx(rhs.m_fsidx),
    m_snds(rhs.m_snds),
    m_commdata(rhs.m_commdata),
    m_src(rhs.m_src),
    m_dst(rhs.m_dst),
    m_srcdm(rhs.m_srcdm),
    m_dstdm(rhs.m_dstdm),
    m_ngrow(rhs.m_ngrow),
    m_reused(rhs.m_reused)
{}

FSRec::~FSRec () {}

bool
FSRec::operator== (const FSRec& rhs) const
{
    return m_ngrow == rhs.m_ngrow &&
        m_src == rhs.m_src        &&
        m_dst == rhs.m_dst        &&
        m_srcdm == rhs.m_srcdm    &&
        m_dstdm == rhs.m_dstdm;
}

bool
FSRec::operator!= (const FSRec& rhs) const
{
    return !operator==(rhs);
}

typedef std::multimap<int,FSRec> FSRecMMap;

typedef FSRecMMap::iterator FSRecMMapIter;

static FSRecMMap TheCache;

void
FabSet::FlushCache ()
{
    int reused = 0;

    for (FSRecMMapIter it = TheCache.begin(); it != TheCache.end(); ++it)
        if (it->second.m_reused)
            reused++;

    if (ParallelDescriptor::IOProcessor() && TheCache.size())
    {
        std::cout << "FabSet::TheCache.size() = " << TheCache.size() << ", # reused = " << reused << '\n';
    }
    TheCache.clear();
}

static
FSRec&
TheFSRec (const MultiFab& src,
          const FabSet&   dst,
          int             ngrow,
          int             scomp,
          int             ncomp)
{
    BL_ASSERT(ngrow >= 0);
    BL_ASSERT(scomp >= 0);
    BL_ASSERT(ncomp >  0);

    static bool first               = true;
    static bool use_copy_cache      = true;
    static int  copy_cache_max_size = 25;   // -1 ==> no maximum size

    if (first)
    {
        first = false;
        ParmParse pp("fabset");
        pp.query("use_copy_cache", use_copy_cache);
        pp.query("copy_cache_max_size", copy_cache_max_size);
    }

    FSRec rec(src.boxArray(),
              dst.boxArray(),
              src.DistributionMap(),
              dst.DistributionMap(),
              ngrow);

    const int key = ngrow + src.size() + dst.size();

    if (use_copy_cache)
    {
        std::pair<FSRecMMapIter,FSRecMMapIter> er_it = TheCache.equal_range(key);

        for (FSRecMMapIter it = er_it.first; it != er_it.second; ++it)
        {
            if (it->second == rec)
            {
                it->second.m_reused = true;
                //
                // Adjust the ncomp & scomp in CommData.
                //
                Array<CommData>& cd = it->second.m_commdata.theCommData();

                for (int i = 0; i < cd.size(); i++)
                {
                    cd[i].nComp(ncomp);
                    cd[i].srcComp(scomp);
                }

                return it->second;
            }
        }

        if (TheCache.size() >= copy_cache_max_size && copy_cache_max_size != -1)
        {
            //
            // Don't let the size of the cache get too big.
            //
            for (FSRecMMapIter it = TheCache.begin(); it != TheCache.end(); )
            {
                if (!it->second.m_reused)
                {
                    TheCache.erase(it++);
                    //
                    // Only delete enough entries to stay under limit.
                    //
                    if (TheCache.size() < copy_cache_max_size) break;
                }
                else
                {
                    ++it;
                }
            }

            if (TheCache.size() >= copy_cache_max_size)
                //
                // Get rid of first entry which is the one with the smallest key.
                //
                TheCache.erase(TheCache.begin());
        }
    }
    else
    {
        TheCache.clear();
    }

    FSRecMMapIter it = TheCache.insert(std::make_pair(key,rec));
    //
    // Calculate and cache intersection info.
    //
    BoxArray ba_src(src.size());
    for (int i = 0; i < src.size(); i++)
        ba_src.set(i, BoxLib::grow(src.boxArray()[i],ngrow));

    for (FabSetIter fsi(dst); fsi.isValid(); ++fsi)
    {
        std::vector< std::pair<int,Box> > isects = ba_src.intersections(dst[fsi].box());

        for (int j = 0; j < isects.size(); j++)
        {
            it->second.m_box.push_back(isects[j].second);
            //
            // Maintain parallel array of indices into MultiFab.
            //
            it->second.m_mfidx.push_back(isects[j].first);
            //
            // Maintain parallel array of indices into FabSet.
            //
            it->second.m_fsidx.push_back(fsi.index());
        }
    }

    return it->second;
}

void
FabSet::DoIt (const MultiFab& src,
              int             ngrow,
              int             scomp,
              int             dcomp,
              int             ncomp,
              How             how)
{
    BL_ASSERT((dcomp+ncomp) <= nComp());
    BL_ASSERT((scomp+ncomp) <= src.nComp());

    BL_ASSERT(how == FabSet::COPYFROM || how == FabSet::PLUSFROM);

    FArrayBox              tmp;
    FabSetCopyDescriptor   fscd;
    std::vector<FillBoxId> fbids;

    FSRec&     fsrec = TheFSRec(src,*this,ngrow,scomp,ncomp);
    MultiFabId mfid  = fscd.RegisterFabArray(const_cast<MultiFab*>(&src));

    BL_ASSERT(fsrec.m_box.size() == fsrec.m_mfidx.size());
    BL_ASSERT(fsrec.m_box.size() == fsrec.m_fsidx.size());

    for (int i = 0; i < fsrec.m_box.size(); i++)
    {
        fbids.push_back(fscd.AddBox(mfid,
                                    fsrec.m_box[i],
                                    0,
                                    fsrec.m_mfidx[i],
                                    scomp,
                                    how == COPYFROM ? dcomp : 0,
                                    ncomp,
                                    false));

        BL_ASSERT(fbids.back().box() == fsrec.m_box[i]);
        //
        // Also save the index of our FAB needing filling.
        //
        fbids.back().FabIndex(fsrec.m_fsidx[i]);
    }

    fscd.CollectData(&fsrec.m_snds, &fsrec.m_commdata);

    for (int i = 0; i < fbids.size(); i++)
    {
        BL_ASSERT(DistributionMap()[fbids[i].FabIndex()] == ParallelDescriptor::MyProc());

        if (how == COPYFROM)
        {
            fscd.FillFab(mfid,fbids[i],(*this)[fbids[i].FabIndex()]);
        }
        else
        {
            tmp.resize(fbids[i].box(), ncomp);

            fscd.FillFab(mfid, fbids[i], tmp);

            (*this)[fbids[i].FabIndex()].plus(tmp,tmp.box(),0,dcomp,ncomp);
        }
    }
}

FabSet&
FabSet::copyFrom (const MultiFab& src,
                  int             ngrow,
                  int             scomp,
                  int             dcomp,
                  int             ncomp)
{
    BL_PROFILE(BL_PROFILE_THIS_NAME() + "::copyFrom()");

    DoIt(src,ngrow,scomp,dcomp,ncomp,FabSet::COPYFROM);

    return *this;
}

FabSet&
FabSet::plusFrom (const MultiFab& src,
                  int             ngrow,
                  int             scomp,
                  int             dcomp,
                  int             ncomp)
{
    BL_PROFILE(BL_PROFILE_THIS_NAME() + "::plusFrom()");

    DoIt(src,ngrow,scomp,dcomp,ncomp,FabSet::PLUSFROM);

    return *this;
}

//
// Linear combination this := a*this + b*src
// Note: corresponding fabsets must be commensurate.
//
FabSet&
FabSet::linComb (Real          a,
                 Real          b,
                 const FabSet& src,
                 int           scomp,
                 int           dcomp,
                 int           ncomp)
{
    BL_ASSERT(size() == src.size());

    for (FabSetIter fsi(*this); fsi.isValid(); ++fsi)
    {
        BL_ASSERT(get(fsi).box() == src[fsi].box());
        //
        // WARNING: same fab used as src and dest here.
        //
        get(fsi).linComb(get(fsi),
                      get(fsi).box(),
                      dcomp,
                      src[fsi],
                      src[fsi].box(),
                      scomp,
                      a,
                      b,
                      get(fsi).box(),
                      dcomp,
                      ncomp);
    }
    return *this;
}

FabSet&
FabSet::linComb (Real            a,
                 const MultiFab& mfa,
                 int             a_comp,
                 Real            b,
                 const MultiFab& mfb,
                 int             b_comp,
                 int             dcomp,
                 int             ncomp,
                 int             ngrow)
{
    BL_ASSERT(ngrow <= mfa.nGrow());
    BL_ASSERT(ngrow <= mfb.nGrow());

    const BoxArray& bxa = mfa.boxArray();

    BL_ASSERT(bxa == mfb.boxArray());

    MultiFabCopyDescriptor mfcd;

    MultiFabId mfid_mfa = mfcd.RegisterFabArray(const_cast<MultiFab*>(&mfa));
    MultiFabId mfid_mfb = mfcd.RegisterFabArray(const_cast<MultiFab*>(&mfb));

    std::vector<FillBoxId> fbids_mfa, fbids_mfb;

    for (FabSetIter fsi(*this); fsi.isValid(); ++fsi)
    {
        for (int grd = 0; grd < bxa.size(); grd++)
        {
            Box ovlp = get(fsi).box() & BoxLib::grow(bxa[grd],ngrow);

            if (ovlp.ok())
            {
                fbids_mfa.push_back(mfcd.AddBox(mfid_mfa,
                                                ovlp,
                                                0,
                                                grd,
                                                a_comp,
                                                0,
                                                ncomp,
                                                false));

                BL_ASSERT(fbids_mfa.back().box() == ovlp);
                //
                // Also save the index of the FAB in the FabSet.
                //
                fbids_mfa.back().FabIndex(fsi.index());

                fbids_mfb.push_back(mfcd.AddBox(mfid_mfb,
                                                ovlp,
                                                0,
                                                grd,
                                                b_comp,
                                                0,
                                                ncomp,
                                                false));

                BL_ASSERT(fbids_mfb.back().box() == ovlp);
            }
        }
    }

    mfcd.CollectData();

    FArrayBox a_fab, b_fab;

    BL_ASSERT(fbids_mfa.size() == fbids_mfb.size());

    for (int i = 0; i < fbids_mfa.size(); i++)
    {
        a_fab.resize(fbids_mfa[i].box(), ncomp);
        b_fab.resize(fbids_mfb[i].box(), ncomp);

        mfcd.FillFab(mfid_mfa, fbids_mfa[i], a_fab);
        mfcd.FillFab(mfid_mfb, fbids_mfb[i], b_fab);

        BL_ASSERT(DistributionMap()[fbids_mfa[i].FabIndex()] == ParallelDescriptor::MyProc());

        (*this)[fbids_mfa[i].FabIndex()].linComb(a_fab,
                                                 fbids_mfa[i].box(),
                                                 0,
                                                 b_fab,
                                                 fbids_mfa[i].box(),
                                                 0,
                                                 a,
                                                 b,
                                                 fbids_mfa[i].box(),
                                                 dcomp,
                                                 ncomp);
    }

    return *this;
}
