! JTTY end-to-end round trip: encode -> audio (+noise) -> rjtty_sub in dataSink-sized steps
! -> jtty_get_updates. Not part of the CMake build; compile by hand against the
! wsjt_fort objects, e.g. gfortran e2e_roundtrip.f90 <objs> -lfftw3f -lfftw3
! args: noise_sigma [signal_Hz [rx_Hz [ftol_Hz]]]   (signal peak amplitude 1000)
! Verified: decodes "CQ TEST DE KA1ABC LIVE AUDIO SMOKE TEST" at noise sigma 2500,
! anywhere in 700-2700 Hz with a wide (passband-wide) window.
program e2e
  implicit none
  integer, parameter :: nsps=384, maxu=30, msglen=80
  character(len=80) :: umsg
  integer :: itone(59*16), nsym, nwave, icmplx, i, k, kz, nfa, nfb, cnt, j, nsps_l, argc
  real :: bt, fsample, f0, ftol, snr_sigma, f0r, ftolr, r1, r2, g, amp
  real, allocatable :: wave(:), cwave(:)
  integer*2, allocatable :: iwave(:)
  integer :: nlead, ntotal, nrun
  character(len=maxu*msglen) :: blocks
  integer*8 :: ids(maxu)
  real :: freqs(maxu), tst(maxu)
  logical*1 :: eom(maxu)
  character(len=msglen) :: last
  integer*8 :: lastid
  character(len=32) :: arg
  umsg='CQ TEST DE KA1ABC LIVE AUDIO SMOKE TEST'
  snr_sigma=300.0
  call getarg(1,arg); if(len_trim(arg).gt.0) read(arg,*) snr_sigma
  bt=2.0; fsample=12000.0; f0=1500.0; icmplx=0
  call getarg(2,arg); if(len_trim(arg).gt.0) read(arg,*) f0
  call genjtty(umsg,itone,nsym)
  nwave=nsps*nsym
  allocate(wave(nwave+nsps*59), cwave(0:nwave-1))
  wave=0.0
  call gen_jttywave(itone,nsym,nsps,bt,fsample,f0,cwave,wave,icmplx,nwave)
  nlead=5*12000
  ntotal=nlead+nwave+20*12000
  allocate(iwave(ntotal)); iwave=0
  amp=1000.0/max(1.0,maxval(abs(wave(1:nwave))))
  call random_seed()
  do i=1,ntotal
    call random_number(r1); call random_number(r2)
    g=sqrt(-2.0*log(max(r1,1e-9)))*cos(6.2831853*r2)
    r1=g*snr_sigma
    if(i.gt.nlead .and. i.le.nlead+nwave) r1=r1+amp*wave(i-nlead)
    iwave(i)=int(max(-32000.0,min(32000.0,r1)),2)
  enddo
  nfa=200; nfb=3000; f0r=1500.0; ftolr=50.0; nsps_l=nsps
  call getarg(3,arg); if(len_trim(arg).gt.0) read(arg,*) f0r
  call getarg(4,arg); if(len_trim(arg).gt.0) read(arg,*) ftolr
  last=''; lastid=0
  do k=12000,ntotal,6000          ! dataSink-like: ~0.5 s of new audio each call
    kz=k
    call rjtty_sub(iwave,kz,nsps_l,nfa,nfb,f0r,ftolr)
    cnt=0
    do
      call jtty_get_updates(blocks,ids,freqs,tst,eom,cnt)
      do j=1,cnt
        if(ids(j).gt.0) then
          last=blocks((j-1)*msglen+1:j*msglen); lastid=ids(j)
          print '(a,i8,a,i12,a,f7.1,a,l1,a,a)','k=',k,' id=',ids(j),' f=',freqs(j),' eom=',eom(j),'  ',trim(last)
        endif
      enddo
      if(cnt.lt.maxu) exit
    enddo
  enddo
  print *,'FINAL: ',trim(last)
  if(trim(adjustl(last)).eq.trim(umsg)) then
    print *,'E2E PASS'
  else
    print *,'E2E MISMATCH'
  endif
end program e2e
