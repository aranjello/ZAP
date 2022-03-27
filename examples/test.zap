var apple = [1,2,3];
apple+apple+apple+apple+apple+apple+apple;
apple = [8,9,10];

??(var a = [5.0]; a; a = a - [1.0]){
    !a;
}

apple = [0,1];
![apple];
?(|apple){
    ?(&apple){
        ![is all true];
    }|{
        ![is partly true];
    }
}|{
    ![is all false];
}

apple = [1,1];
![apple];
!apple;
?(&apple){
    ![not];
}|{
    ![is];
}
