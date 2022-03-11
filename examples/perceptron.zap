var input = [[1,0,1]
            ,[1,1,0]
            ,[0,1,0]
            ,[0,0,0]]
!input
var output =[1,1,0,0]
!output
var weights = [.35,.45,.11]

!weights

var tout = input.weights

var error =  tout - output

weights = weights+input.(error*tout)

!weights

![1,1,1]*weights