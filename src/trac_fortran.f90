PROGRAM trac_fortran
  USE iso_c_binding
  USE mptrac_struct
  USE mptrac_func
  USE, intrinsic :: iso_fortran_env
  IMPLICIT NONE

  CHARACTER(len=40) :: filename_ctl, filename_atm, dirname
  CHARACTER(len=5), DIMENSION(5) :: argv
  INTEGER :: argc
  TYPE(ctl_t) :: ctl
  TYPE(atm_t) :: atm
  TYPE(clim_t) :: clim
  TYPE(met_t), TARGET :: met0, met1
  REAL(real64) :: t
  REAL(real64), DIMENSION(npp) :: dt

  filename_ctl = "data/trac.ctl"
  filename_atm = "atm_split.tab"

  argc = 5
  argv(1:5) = "abc"

  ! Read control parameters... 
  CALL mptrac_read_ctl(TRIM(filename_ctl)//char(0), argc, TRIM(argv(1))//char(0), ctl)

  ! Read climatological data... 
  CALL mptrac_read_clim(ctl, clim)
  
  ! Read atmospheric data...
  CALL mptrac_read_atm(TRIM(filename_atm)//char(0), ctl, atm)

  ! Initialize timesteps...
  CALL mptrac_module_timesteps_init(ctl, atm)

  WRITE(*,*) ctl%t_start
  ! Get meteo data...
  CALL mptrac_get_met(ctl, clim, ctl%t_start, met0, met1)

  dirname = "data"

  t = ctl%t_start

  DO WHILE (ctl%direction * (t - ctl%t_stop) < ctl%dt_mod)

     ! Adjust length of final time step... 
     IF (ctl%direction * (t - ctl%t_stop) > 0) THEN
        t = ctl%t_stop
     ENDIF

     ! Set time steps of air parcels...
     CALL mptrac_module_timesteps(ctl, met0, atm, dt, t)

     IF (t .NE. ctl%t_start) THEN
        ! Get meteo data...
        CALL mptrac_get_met(ctl, clim, t, met0, met1)
     ENDIF

     ! Advection...
     CALL mptrac_module_advect(ctl, met0, met1, atm, dt)

     ! Write output...
     CALL mptrac_write_output(TRIM(dirname)//char(0), ctl, met0, met1, atm, t)

     t = t + ctl%direction * ctl%dt_mod

  END DO

END PROGRAM trac_fortran

  
