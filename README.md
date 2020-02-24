# Receipt parser

Receipt parser app, using OCR (Tesseract) and regular expressions, with BK-tree based spellchecker. 

## Overview

The input of the app is an image of a receipt. Tesseract library is used for optical character recognition. 
The obtained text is processed by a parser, that recognizes rows containing products and the sum.

A spellchecker The app comes with a dictionary
