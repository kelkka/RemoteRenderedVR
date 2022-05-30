
# Files

## CSV files

- `Experiment_Movement.csv`
Contains per frame information Time deltas, MSE-differences and movement data for each 464 clips. Each clip is separated by two rows containing the header-information and a "per file"-metadata. The rows following these two lines are then the data for that clip, centered around the freeze-event. 


Example of the two header-rows and two data rows here; 

```
Frame,MSE,R_delta,T_delta,Frozen,R_x,R_y,R_z,T_x,T_y,T_z,Time,File,TrackErr,Correct,Answer,ScoreErr,Annoy,Difficulty,FreezeLen,MSE_0,MSE_1,MSE_2,MSE_3,Rot_0,Rot_1,Trans_0,Trans_1,Task,Index,Type,,,,,
-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,2022-3-3-13:29:30,../QOE/FreezeTest/FreezeType_Reprojection/08A.txt,0,1,1,0,4,4,8,0.041849,0,0.040941,0.041609,0.021195,0.021195,0.001989,0.001989,1,1,Reprojection,,,,,
329,0.0389633,0.0269853,0.000561524,0,-0.286627,-0.0712616,0.265868,-0.347403,-0.0712616,0.265868,,,,,,,,,,,,,,,,,,,,,,,,,
330,0.0386724,0.0269162,0.000513614,0,-0.313336,-0.0735332,0.268309,-0.347171,-0.0735332,0.268309,,,,,,,,,,,,,,,,,,,,,,,,,
```
  
- `Experiment_Summary.csv`

Per clip summary data for each 464 clips, starting with a header-row. 

- `SSQ.csv`

Simulator Sickness Questionnaire results before and after test, for each subject. Assessment translated into numbers for easier analysis. Key/legend can be found in `Legend_SSQ.csv`.

- `Legend_SSQ.csv`

Key/legend for the numbered assessment in the `SSQ.csv`-file

- `Subject_Metadata.csv`

Per-subject metadata about age, gender and VR-experience, together with an anonymized user ID that follows through the dataset.


## Excel files

- `QoE_Data.xlsx` contains all data and a number of plots for data visualization. 
