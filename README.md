IMRT Solver
-----------

**Cómo correr un ejemplo:**

Compilar:

cmake .
make

Para correr:

´´´
  ./IAS {OPTIONS}

    ********* IMRT-Solver (Intensity-aperture solver) *********

  OPTIONS:

      -h, --help                        Display this help menu
      --bsize=[int]                     Number of considered beamlets for
                                        selection (5)
      --vsize=[int]                     Number of considered worst voxels (20)
      --int0=[int]                      Initial intensity for beams (4.000000)
      --max_ap=[int]                    Initial intensity for the station (4)
      --maxdelta=[int]                  Max delta (5.000000)
      --maxratio=[int]                  Max ratio (3.000000)
      --alpha=[double]                  Initial temperature for intensities
                                        (1.000000)
      --beta=[double]                   Initial temperature for ratio (1.000000)
      --max_iter=[int]                  Number of iterations (100)

    An IMRT Solver.
´´´

Por ejemplo:

     ./IAS --max_iter=400 --maxdelta=8 --maxratio=6 --alpha=0.999 --beta=0.999 --max_ap=4 --help

----

##El Algoritmo


´´´
S ← initializeStations (max_apertures, initial_intensity)
bestF ← eval(S)

while i < max_iter:
   (b,si,increase) ← select_promising_beamlet(bsize,vsize)
   delta_intensity ← eα (max_iter/i)
   ratio ← eβ (max_iter/i)

   if increase:
     diff ← increaseIntensity&Repair (b,si,delta_intenisty,ratio)
   else:
     diff ← decreaseIntensity&Repair (b,si,delta_intenisty,ratio)

   F ← incremental_eval (S, diff)

   if F < bestF:
     bestF ← F
     bestS ← S
   else
     revert_changes(s,diff)
     undo_last_eval()
   
´´´
[Más detalles](https://docs.google.com/document/d/1EGoKoLsmik4TSiY_SslWkxddDCrYvUfFKpPLRpQPV_U/edit#)



