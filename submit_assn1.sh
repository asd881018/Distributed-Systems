#!/bin/bash
#
#SBATCH --cpus-per-task=8
#SBATCH --time=02:00
#SBATCH --mem=1G
#SBATCH --partition=fast

#srun /home/$USER/CMPT431/assignments/assignment0/producer_consumer
# srun /home/$USER/CMPT431/assignments/assignment0/producer_consumer --nItems 100000 --nProducers 5 --nConsumers 3 --bufferSize 100000
# srun python /scratch/assignment0/test_scripts/solution_tester.pyc --execPath=/home/cta106/CMPT431/assignments/assignment0/producer_consumer
srun python /scratch/assignment0/test_scripts/submission_validator.pyc --tarPath=/home/cta106/CMPT431/assignments/assignment0/assignment0.tar.gz
