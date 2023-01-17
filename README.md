# DMC (Deterministic Markov Chain) 
DMC is an open-source project that aims to create a new approach to text generation using Markov chains. The goal of the project is to create a Markov chain that can be ordered around to tell what context to tell. The project is based on the idea of transforming the Markov chain into a 2D plane where words with similar context are close to each other, making it easier to traverse.

## Problem
The main problem that DMC aims to solve is the difficulty of traversing linear paths in natural language, where the current word has some probabilities to hop into the next word and thus make sentences. DMC aims to overcome this problem by creating a 2D gradient where the similarity of a word gets less and less the farther away it is from another word.

## Solution
To achieve this goal, the project utilizes dimensionality reduction techniques such as PCA and t-SNE to create this 2D gradient. Additionally, the project also explores the use of clustering algorithms and the vector space model to group words with similar context together.

## Restrictions
The project is built without the use of neural networks and the main reason for this is to find an equation for text generation like neural networks do.

In summary, DMC is an open-source project that aims to create a new approach to text generation using Markov chains. The project seeks to overcome the difficulties of traversing linear paths in natural language by creating a 2D gradient of words with similar context. The project utilizes dimensionality reduction techniques, clustering algorithms and the vector space model to achieve this goal.