#pragma once

//Toroidally linked matrix for solving sudoku puzzles via algorithm x
//Optimized to only work with standard 9x9 puzzles
class DLinks {
	public:
	typedef enum {
		h,	// horizontal: left and right
		v	// vertical: up and down
	} HV;

	class Node {
		public:
		int row, col, count;
		Node *up, *down, *left, *right;

		template<bool cut = true>
		inline void insert_after_h(Node *ante) {
			if ( cut ) {
				this->right->left = this->left;
				this->left->right = this->right;
			}
			this->right = ante->right;
			this->left  = ante;
			ante->right->left = this;
			ante->right = this;
		}
		inline void connect_after_v(Node *ante) {
			ante->down	= this;
			this->up	= ante;
		}
		inline void selfloop_h() {
			this->left = this->right = this;
		}
		template<HV hv>
		inline void disconnect() {
			if ( hv == h ) {
				this->right->left = this->left;
				this->left->right = this->right;
			} else {
				this->down->up = this->up;
				this->up->down = this->down;
			}
		}
		// inverse operation to disconnect<v>,
		// provided the node preserved the prior up and down links
		inline void reconnect_v() {
			this->down->up = this;
			this->up->down = this;
		}
	};

	Node cols[324][10];
	Node counts[11];
	Node* solution_stack[81];
	int solution_ptr;

	inline void init() {
		//initialize matrix
		for(int i=0; i<324; ++i) {
			cols[i]->count = 0;
		}
		for(int i=0; i<11; ++i) {
			counts[i].selfloop_h();
		}

		solution_ptr = 0;
	}

	//finalize toroidal structure of columns
	inline void finalize_cols() {
		Node* n1;
		for(int i=0; i<324; ++i) {
			n1 = cols[i];
			n1->connect_after_v(n1+n1->count);
		}
	}

	//assign the column headers to their respective lists based on node count
	inline void assign_column_headers() {
		Node* n1;
		for(int i=0; i<324; ++i) {
			n1 = cols[i];
			if(n1->count > 9) { continue; }
			n1->insert_after_h<false>(counts+n1->count);
		}
	}

	//selects next available node in specified column
	//updates node attributes, updates node.up and node.up.down ptrs
	//returns pointer to selected node
	inline Node* insert(int row, int col) {
		Node* n1 = cols[col];
		int &ptr = n1->count;
		Node* insert = n1+ptr+1;
		insert->row = row;
		insert->col = col;
		insert->connect_after_v(n1+ptr);
		++ptr;
		return insert;
	}

	// return a column containing the minimum uncovered nodes
	// Note: in fact, just pick the first uncovered column
	//if all columns are covered - return NULL
	inline Node* select_min_column() {
		for(int i=0; i<10; ++i) {
			if(counts[i].right != &counts[i]) {
				return counts[i].right;
			}
		}
		return 0;
	}

	//cover a column of node n for dancing links algorithm
	inline void cover(Node* n) {
		Node* col_node = cols[n->col];
		//unlink left and right neighbors of col from col
		col_node->disconnect<h>();
		//iterate through each Node in col top to bottom
		for(Node* vert_itr=col_node->down; vert_itr!=col_node; vert_itr=vert_itr->down) {
			//iterate through row left to right
			//for each Node in this row, unlink top and bottom neighbors and reduce count of that column
			for(Node* horiz_itr=vert_itr->right; horiz_itr!=vert_itr; horiz_itr=horiz_itr->right) {
				horiz_itr->disconnect<v>();
				Node* cn = cols[horiz_itr->col];
				--cn->count;
				//if column cn is not covered - push it into new list
				if(cn->right->left == cn) {
					cn->insert_after_h(counts+cn->count);
				}
			}
		}
	}

	//uncover a column of node n for dancing links algorithm
	inline void uncover(Node* n) {
		Node* col_node = cols[n->col];
		//relink left and right neighbors of col to col
		col_node->insert_after_h<false>(counts+col_node->count);

		//iterate through cols bottom to top
		for(Node* vert_itr=col_node->up; vert_itr!=col_node; vert_itr=vert_itr->up) {
			//iterate through row right to left
			//for each Node in this row, relink top and bottom neighbors and increment count of that column
			for(Node* horiz_itr=vert_itr->left; horiz_itr!=vert_itr; horiz_itr=horiz_itr->left) {
				horiz_itr->reconnect_v();
				Node* cn = cols[horiz_itr->col];
				++cn->count;
				//if column cn is not covered - push it into new list
				if(cn->right->left == cn) {
					cn->insert_after_h(counts+cn->count);
				}
			}
		}
	}

#if 0
	//returns true if exact cover is found, false otherwise
	//solutions_stack will contain all rows making up the solution
	bool alg_x_rec_search(){
		//if no column in matrix, then a solution has been found
		Node* selected_col = select_min_column();
		if(selected_col == 0) {
			return true;
		}
		//if selected column has 0 Nodes, then this branch has failed
		if(selected_col->count < 1) {
			return false;
		}

		Node* vert_itr, *horiz_itr;
		//iterate down from selected column head
		for(vert_itr=selected_col->down; vert_itr!=selected_col; vert_itr=vert_itr->down) {
			//add selected row to solutions LIFO
			solution_stack[solution_ptr] = vert_itr;
			++solution_ptr;
				horiz_itr = vert_itr;
			//iterate right from vertical iterator, cover each column
			do {
				cover(horiz_itr);
			} while((horiz_itr = horiz_itr->right) != vert_itr);

			//search this matrix again after covering
			//if solution found on this branch, leave loop and stop searching
			if( alg_x_rec_search() ) {
				 return true;
			}

			//solution not found on this iteration's branch, need to revert changes to matrix
			//remove row from solutions, then uncover columns from this iteration
			--solution_ptr;
			horiz_itr = vert_itr->left;
			//iterate left from the last column that was covered, uncover each column
			do {
				uncover(horiz_itr);
			} while((horiz_itr = horiz_itr->left) != vert_itr->left);
		}
		return false;
	}
#endif

	//iterative implementation of the search
	inline bool alg_x_itr_search(int num_start_sols) {
		//select initial column to begin the search
		Node* selected_col, *vert_itr, *horiz_itr;
		if((selected_col = select_min_column())->count < 1) {
			return false;
		}

		vert_itr = selected_col->down;
		while(true) {
			//select current row as partial solution and cover
			solution_stack[solution_ptr++] = vert_itr;
			horiz_itr = vert_itr;
			do {
				cover(horiz_itr);
			} while((horiz_itr = horiz_itr->right) != vert_itr);

			//select next column and check for solution, and branch failure
			if((selected_col = select_min_column()) == 0) {
				return true;
			}
			if(selected_col->count > 0) {
				vert_itr = selected_col->down;
				continue;
			}

			//uncover last partial solution
			do {
				if(--solution_ptr < num_start_sols) { return false; }
				vert_itr  = solution_stack[solution_ptr];
				horiz_itr = vert_itr->left;
				do{
					uncover(horiz_itr);
				} while ( (horiz_itr = horiz_itr->left) != vert_itr->left );
				vert_itr = vert_itr->down;
				//if next column is a header, then continue to uncover
			} while(vert_itr == cols[vert_itr->up->col] );
		}
	}

	//cover the known solutions to the puzzle when initializing the matrix
	void initial_cover(Node* c){
		c->count = 100;
		for(Node* vert_itr=c->down; vert_itr!=c; vert_itr=vert_itr->down) {
			for(Node* horiz_itr=vert_itr->right; horiz_itr!=vert_itr; horiz_itr=horiz_itr->right) {
				horiz_itr->disconnect<v>();
				--cols[horiz_itr->col]->count;
			}
		}
	}
};

// lookup tables for the 4 constraints of a 9x9 sudoku
const int one_c[] = { 0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,10,10,10,10,10,10,10,10,10,11,11,11,11,11,11,11,11,11,12,12,12,12,12,12,12,12,12,13,13,13,13,13,13,13,13,13,14,14,14,14,14,14,14,14,14,15,15,15,15,15,15,15,15,15,16,16,16,16,16,16,16,16,16,17,17,17,17,17,17,17,17,17,18,18,18,18,18,18,18,18,18,19,19,19,19,19,19,19,19,19,20,20,20,20,20,20,20,20,20,21,21,21,21,21,21,21,21,21,22,22,22,22,22,22,22,22,22,23,23,23,23,23,23,23,23,23,24,24,24,24,24,24,24,24,24,25,25,25,25,25,25,25,25,25,26,26,26,26,26,26,26,26,26,27,27,27,27,27,27,27,27,27,28,28,28,28,28,28,28,28,28,29,29,29,29,29,29,29,29,29,30,30,30,30,30,30,30,30,30,31,31,31,31,31,31,31,31,31,32,32,32,32,32,32,32,32,32,33,33,33,33,33,33,33,33,33,34,34,34,34,34,34,34,34,34,35,35,35,35,35,35,35,35,35,36,36,36,36,36,36,36,36,36,37,37,37,37,37,37,37,37,37,38,38,38,38,38,38,38,38,38,39,39,39,39,39,39,39,39,39,40,40,40,40,40,40,40,40,40,41,41,41,41,41,41,41,41,41,42,42,42,42,42,42,42,42,42,43,43,43,43,43,43,43,43,43,44,44,44,44,44,44,44,44,44,45,45,45,45,45,45,45,45,45,46,46,46,46,46,46,46,46,46,47,47,47,47,47,47,47,47,47,48,48,48,48,48,48,48,48,48,49,49,49,49,49,49,49,49,49,50,50,50,50,50,50,50,50,50,51,51,51,51,51,51,51,51,51,52,52,52,52,52,52,52,52,52,53,53,53,53,53,53,53,53,53,54,54,54,54,54,54,54,54,54,55,55,55,55,55,55,55,55,55,56,56,56,56,56,56,56,56,56,57,57,57,57,57,57,57,57,57,58,58,58,58,58,58,58,58,58,59,59,59,59,59,59,59,59,59,60,60,60,60,60,60,60,60,60,61,61,61,61,61,61,61,61,61,62,62,62,62,62,62,62,62,62,63,63,63,63,63,63,63,63,63,64,64,64,64,64,64,64,64,64,65,65,65,65,65,65,65,65,65,66,66,66,66,66,66,66,66,66,67,67,67,67,67,67,67,67,67,68,68,68,68,68,68,68,68,68,69,69,69,69,69,69,69,69,69,70,70,70,70,70,70,70,70,70,71,71,71,71,71,71,71,71,71,72,72,72,72,72,72,72,72,72,73,73,73,73,73,73,73,73,73,74,74,74,74,74,74,74,74,74,75,75,75,75,75,75,75,75,75,76,76,76,76,76,76,76,76,76,77,77,77,77,77,77,77,77,77,78,78,78,78,78,78,78,78,78,79,79,79,79,79,79,79,79,79,80,80,80,80,80,80,80,80,80,};
const int row_c[] = { 81,82,83,84,85,86,87,88,89,81,82,83,84,85,86,87,88,89,81,82,83,84,85,86,87,88,89,81,82,83,84,85,86,87,88,89,81,82,83,84,85,86,87,88,89,81,82,83,84,85,86,87,88,89,81,82,83,84,85,86,87,88,89,81,82,83,84,85,86,87,88,89,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,90,91,92,93,94,95,96,97,98,90,91,92,93,94,95,96,97,98,90,91,92,93,94,95,96,97,98,90,91,92,93,94,95,96,97,98,90,91,92,93,94,95,96,97,98,90,91,92,93,94,95,96,97,98,90,91,92,93,94,95,96,97,98,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,99,100,101,102,103,104,105,106,107,99,100,101,102,103,104,105,106,107,99,100,101,102,103,104,105,106,107,99,100,101,102,103,104,105,106,107,99,100,101,102,103,104,105,106,107,99,100,101,102,103,104,105,106,107,99,100,101,102,103,104,105,106,107,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,108,109,110,111,112,113,114,115,116,108,109,110,111,112,113,114,115,116,108,109,110,111,112,113,114,115,116,108,109,110,111,112,113,114,115,116,108,109,110,111,112,113,114,115,116,108,109,110,111,112,113,114,115,116,108,109,110,111,112,113,114,115,116,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,117,118,119,120,121,122,123,124,125,117,118,119,120,121,122,123,124,125,117,118,119,120,121,122,123,124,125,117,118,119,120,121,122,123,124,125,117,118,119,120,121,122,123,124,125,117,118,119,120,121,122,123,124,125,117,118,119,120,121,122,123,124,125,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,126,127,128,129,130,131,132,133,134,126,127,128,129,130,131,132,133,134,126,127,128,129,130,131,132,133,134,126,127,128,129,130,131,132,133,134,126,127,128,129,130,131,132,133,134,126,127,128,129,130,131,132,133,134,126,127,128,129,130,131,132,133,134,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,135,136,137,138,139,140,141,142,143,135,136,137,138,139,140,141,142,143,135,136,137,138,139,140,141,142,143,135,136,137,138,139,140,141,142,143,135,136,137,138,139,140,141,142,143,135,136,137,138,139,140,141,142,143,135,136,137,138,139,140,141,142,143,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,144,145,146,147,148,149,150,151,152,144,145,146,147,148,149,150,151,152,144,145,146,147,148,149,150,151,152,144,145,146,147,148,149,150,151,152,144,145,146,147,148,149,150,151,152,144,145,146,147,148,149,150,151,152,144,145,146,147,148,149,150,151,152,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,153,154,155,156,157,158,159,160,161,153,154,155,156,157,158,159,160,161,153,154,155,156,157,158,159,160,161,153,154,155,156,157,158,159,160,161,153,154,155,156,157,158,159,160,161,153,154,155,156,157,158,159,160,161,153,154,155,156,157,158,159,160,161,153,154,155,156,157,158,159,160,161};
const int col_c[] = { 162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242};
const int box_c[] = { 243,244,245,246,247,248,249,250,251,243,244,245,246,247,248,249,250,251,243,244,245,246,247,248,249,250,251,252,253,254,255,256,257,258,259,260,252,253,254,255,256,257,258,259,260,252,253,254,255,256,257,258,259,260,261,262,263,264,265,266,267,268,269,261,262,263,264,265,266,267,268,269,261,262,263,264,265,266,267,268,269,243,244,245,246,247,248,249,250,251,243,244,245,246,247,248,249,250,251,243,244,245,246,247,248,249,250,251,252,253,254,255,256,257,258,259,260,252,253,254,255,256,257,258,259,260,252,253,254,255,256,257,258,259,260,261,262,263,264,265,266,267,268,269,261,262,263,264,265,266,267,268,269,261,262,263,264,265,266,267,268,269,243,244,245,246,247,248,249,250,251,243,244,245,246,247,248,249,250,251,243,244,245,246,247,248,249,250,251,252,253,254,255,256,257,258,259,260,252,253,254,255,256,257,258,259,260,252,253,254,255,256,257,258,259,260,261,262,263,264,265,266,267,268,269,261,262,263,264,265,266,267,268,269,261,262,263,264,265,266,267,268,269,270,271,272,273,274,275,276,277,278,270,271,272,273,274,275,276,277,278,270,271,272,273,274,275,276,277,278,279,280,281,282,283,284,285,286,287,279,280,281,282,283,284,285,286,287,279,280,281,282,283,284,285,286,287,288,289,290,291,292,293,294,295,296,288,289,290,291,292,293,294,295,296,288,289,290,291,292,293,294,295,296,270,271,272,273,274,275,276,277,278,270,271,272,273,274,275,276,277,278,270,271,272,273,274,275,276,277,278,279,280,281,282,283,284,285,286,287,279,280,281,282,283,284,285,286,287,279,280,281,282,283,284,285,286,287,288,289,290,291,292,293,294,295,296,288,289,290,291,292,293,294,295,296,288,289,290,291,292,293,294,295,296,270,271,272,273,274,275,276,277,278,270,271,272,273,274,275,276,277,278,270,271,272,273,274,275,276,277,278,279,280,281,282,283,284,285,286,287,279,280,281,282,283,284,285,286,287,279,280,281,282,283,284,285,286,287,288,289,290,291,292,293,294,295,296,288,289,290,291,292,293,294,295,296,288,289,290,291,292,293,294,295,296,297,298,299,300,301,302,303,304,305,297,298,299,300,301,302,303,304,305,297,298,299,300,301,302,303,304,305,306,307,308,309,310,311,312,313,314,306,307,308,309,310,311,312,313,314,306,307,308,309,310,311,312,313,314,315,316,317,318,319,320,321,322,323,315,316,317,318,319,320,321,322,323,315,316,317,318,319,320,321,322,323,297,298,299,300,301,302,303,304,305,297,298,299,300,301,302,303,304,305,297,298,299,300,301,302,303,304,305,306,307,308,309,310,311,312,313,314,306,307,308,309,310,311,312,313,314,306,307,308,309,310,311,312,313,314,315,316,317,318,319,320,321,322,323,315,316,317,318,319,320,321,322,323,315,316,317,318,319,320,321,322,323,297,298,299,300,301,302,303,304,305,297,298,299,300,301,302,303,304,305,297,298,299,300,301,302,303,304,305,306,307,308,309,310,311,312,313,314,306,307,308,309,310,311,312,313,314,306,307,308,309,310,311,312,313,314,315,316,317,318,319,320,321,322,323,315,316,317,318,319,320,321,322,323,315,316,317,318,319,320,321,322,323};

//convert char array representing puzzle into constraint matrix for algorithm x
// mask_row and mask_col, if given, represent the set values in their row or col as a bit mask
inline bool solve_puzzle(DLinks *dl, unsigned char* sudoku_list, unsigned short *mask_row, unsigned short *mask_col){
	using Node = DLinks::Node;

	Node* init_covered[324];
	int init_ptr = 0;

	dl->init();
	//iterate through sudoku list
	int row = 0;
	for(int i=0; i<81; ++i) {
		Node *n1;
		int single = 0;

		// if no value assigned to cell, populate all rows representing all possible candidate values for cell
		// except for those already set in the row or col
		unsigned int cands = sudoku_list[i] == '0'? 0x1ff : sudoku_list[i] == '.'? 0x1ff : sudoku_list[i]-'0';
		if( cands == 0x1ff) {	// i.e. all candidates as bitmask.
			if ( mask_row ) {
				cands &= ~mask_row[i/9];
			}
			if ( mask_col ) {
				cands &= ~mask_col[i%9];
			}
		} else {
			single = cands;
		}
		//for the bits in the given sudoku candidates, populate rows representing the possible candidates
		if ( single == 0 ) {
			for(unsigned int j=0; j<9; ++j) {
				if ( (1<<j) & cands ) {
					row = i*9+j;
					if ( cands == (1U<<j) ) {
						single = j+1;
						break;
					} else {
						Node* n1 = dl->insert(row, one_c[row]);
						Node* n2 = dl->insert(row, row_c[row]);
						Node* n3 = dl->insert(row, col_c[row]);
						Node* n4 = dl->insert(row, box_c[row]);
						n1->right = n2; n2->right = n3; n3->right = n4; n4->right = n1;
						n4->left = n3; n3->left = n2; n2->left = n1; n1->left = n4;
					}
				}
			}
		}
		if ( single ) {
			row = i*9+single-1;

			// mark each of the 4 columns as part of the initial solution
			init_covered[init_ptr++] = &dl->cols[one_c[row]][0];
			init_covered[init_ptr++] = &dl->cols[row_c[row]][0];
			init_covered[init_ptr++] = &dl->cols[col_c[row]][0];
			init_covered[init_ptr++] = &dl->cols[box_c[row]][0];

			Node *p = dl->cols[one_c[row]];
			n1 = &p[p->count+1];
			n1->row = row;
			dl->solution_stack[dl->solution_ptr++] = n1;
		}
	}

	//finalize toroidal structure of columns
	dl->finalize_cols();

	//cover each column of known partial solutions
	for(int i=0; i<init_ptr; ++i) {
		dl->initial_cover(init_covered[i]);
	}

	dl->assign_column_headers();

	return dl->alg_x_itr_search(dl->solution_ptr);
	//return dl->alg_x_rec_search();
}

inline bool solve_puzzle(DLinks * dl, unsigned char * puzzle) {
	unsigned short mask_row[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	unsigned short mask_col[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    for(int i=0; i<81; ++i) {
		if ( puzzle[i] == '0' ||  puzzle[i] == '.' ) {
			continue;
		}
		int single_bit = 1<<(puzzle[i] - '1');
		mask_row[i/9] |= single_bit;
		mask_col[i%9] |= single_bit;
	}
	return solve_puzzle(dl, puzzle, mask_row, mask_col);
}