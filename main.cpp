#include <bits/stdc++.h>
using namespace std;

// Bookstore Management System - partial implementation
// Focus: Account System and basic Book System to pass public tests

struct Account {
    string userid;
    string password;
    string username;
    int privilege; // 1,3,7
};

struct Session { string userid; int privilege; string selected_isbn; };

struct Book {
    string isbn;
    string name;
    string author;
    string keyword; // segments joined by '|'
    double price = 0.0;
    long long stock = 0;
};

static const string ACC_FILE = "accounts.dat";
static const string LOGIN_FILE = "login.dat"; // optional persistence of login stack
static const string BOOK_FILE = "books.dat";

// ---------------- Utilities ----------------
static bool is_valid_userid(const string &s){
    if(s.size()>30) return false;
    for(char c: s){ if(!(isalnum((unsigned char)c) || c=='_')) return false; }
    return true;
}
static bool is_valid_password(const string &s){
    if(s.size()>30) return false;
    for(char c: s){ if(!(isalnum((unsigned char)c) || c=='_')) return false; }
    return true;
}
static bool is_valid_username(const string &s){
    if(s.size()>30) return false;
    for(char c: s){ if(c=='\"' || c=='\n' || c=='\r') return false; }
    return true;
}
static bool is_valid_priv(const string &s){
    if(s.size()!=1) return false;
    if(s!="1" && s!="3" && s!="7") return false;
    return true;
}
static bool is_valid_isbn(const string &s){
    if(s.size()>20) return false;
    for(char c: s){ if(c=='\n' || c=='\r') return false; }
    return true;
}
static bool is_valid_named_field(const string &s){ // name/author
    if(s.size()>60) return false;
    for(char c: s){ if(c=='\"' || c=='\n' || c=='\r') return false; }
    return true;
}
static bool is_valid_keyword_all(const string &s){
    if(s.size()>60) return false;
    for(char c: s){ if(c=='\"' || c=='\n' || c=='\r') return false; }
    return true;
}
static bool has_unique_keyword_segments(const string &s){
    unordered_set<string> segs;
    size_t start=0;
    while(true){
        size_t pos = s.find('|', start);
        string part = s.substr(start, pos==string::npos? string::npos : pos-start);
        if(part.empty()) return false;
        if(segs.count(part)) return false;
        segs.insert(part);
        if(pos==string::npos) break;
        start=pos+1;
    }
    return true;
}
static bool is_positive_integer(const string &s){
    if(s.empty()) return false;
    for(char c: s){ if(!isdigit((unsigned char)c)) return false; }
    // no need to check max value here
    return s != "0";
}
static bool is_positive_number(const string &s){
    if(s.empty()) return false;
    int dots=0; for(char c: s){ if(!(isdigit((unsigned char)c) || c=='.')) return false; dots += (c=='.'); }
    if(dots>1) return false;
    try{ double v=stod(s); return v>0; }catch(...){ return false; }
}

// ---------------- Persistence ----------------
static vector<Account> load_accounts(){
    vector<Account> v;
    ifstream fin(ACC_FILE);
    if(!fin){
        Account root{"root","sjtu","root",7};
        v.push_back(root);
        ofstream fout(ACC_FILE);
        fout << root.userid << "," << root.password << "," << root.username << "," << root.privilege << "\n";
        return v;
    }
    string line;
    while(getline(fin,line)){
        if(line.empty()) continue;
        size_t p1 = line.find(','); if(p1==string::npos) continue;
        size_t p2 = line.find(',', p1+1); if(p2==string::npos) continue;
        size_t plast = line.rfind(','); if(plast==string::npos || plast==p2) continue;
        string u = line.substr(0, p1);
        string p = line.substr(p1+1, p2-(p1+1));
        string n = line.substr(p2+1, plast-(p2+1));
        string pr = line.substr(plast+1);
        if(u.empty()) continue;
        try{
            int priv = stoi(pr);
            v.push_back(Account{u,p,n,priv});
        }catch(...){ /* skip */ }
    }
    return v;
}
static void save_accounts(const vector<Account>& v){
    ofstream fout(ACC_FILE, ios::trunc);
    for(const auto &a: v){
        fout << a.userid << "," << a.password << "," << a.username << "," << a.privilege << "\n";
    }
}

static vector<Session> load_login(){
    vector<Session> s;
    ifstream fin(LOGIN_FILE);
    string line;
    while(getline(fin,line)){
        if(line.empty()) continue;
        string u,pr,sel;
        size_t p1=line.find(','); if(p1==string::npos) continue;
        size_t p2=line.find(',', p1+1); if(p2==string::npos) continue;
        u=line.substr(0,p1); pr=line.substr(p1+1,p2-(p1+1)); sel=line.substr(p2+1);
        try{ int piv=stoi(pr); s.push_back({u,piv,sel}); }catch(...){ }
    }
    return s;
}
static void save_login(const vector<Session>& s){
    ofstream fout(LOGIN_FILE, ios::trunc);
    for(auto &x: s){ fout << x.userid << "," << x.privilege << "," << x.selected_isbn << "\n"; }
}

static vector<Book> load_books(){
    vector<Book> v;
    ifstream fin(BOOK_FILE);
    string line;
    while(getline(fin,line)){
        if(line.empty()) continue;
        vector<string> f; f.reserve(6);
        size_t start=0; for(int i=0;i<5;i++){ size_t pos=line.find('\t', start); if(pos==string::npos){ f.clear(); break; } f.push_back(line.substr(start,pos-start)); start=pos+1; }
        if(f.empty()) continue; // malformed
        string stock_str = line.substr(start);
        if(f.size()!=5) continue;
        try{
            double price = stod(f[4]);
            long long stock = stoll(stock_str);
            v.push_back(Book{f[0], f[1], f[2], f[3], price, stock});
        }catch(...){ }
    }
    return v;
}
static void save_books(const vector<Book>& v){
    ofstream fout(BOOK_FILE, ios::trunc);
    for(const auto &b: v){
        fout << b.isbn << '\t' << b.name << '\t' << b.author << '\t' << b.keyword << '\t' << fixed << setprecision(2) << b.price << '\t' << b.stock << "\n";
    }
}

static Account* find_account(vector<Account>& v, const string& uid){
    for(auto &a: v) if(a.userid==uid) return &a; return nullptr;
}
static Book* find_book(vector<Book>& v, const string& isbn){
    for(auto &b: v){ if(b.isbn==isbn) return &b; }
    return nullptr;
}
static int current_priv(const vector<Session>& stack){
    if(stack.empty()) return 0; else return stack.back().privilege;
}

// Tokenizer: respects quoted strings as single token when starting with a quote; mid-token quotes are kept
static vector<string> tokenize(const string &line){
    vector<string> t;
    int i=0,n=line.size();
    while(i<n){
        while(i<n && line[i]==' ') i++;
        if(i>=n) break;
        if(line[i]=='\"'){
            i++; string buf; while(i<n){ if(line[i]=='\"'){ i++; break;} buf.push_back(line[i++]); }
            t.push_back(buf);
        }else{
            string buf;
            while(i<n && line[i]!=' ') buf.push_back(line[i++]);
            t.push_back(buf);
        }
    }
    return t;
}

int main(){
    ios::sync_with_stdio(false); cin.tie(nullptr);
    vector<Account> accounts = load_accounts();
    vector<Book> books = load_books();
    vector<Session> stack = {}; // fresh login stack per run

    string line;
    while(true){
        if(!getline(cin,line)) break;
        // Strip optional leading numbering like "123→" or "123->"
        auto strip_prefix = [&](string &s){
            auto all_digits_spaces = [&](const string &a, size_t end){ for(size_t i=0;i<end;i++){ if(!(isdigit((unsigned char)a[i]) || a[i]==' ')) return false; } return end>0; };
            const string utf_arrow = u8"→";
            size_t p = s.find(utf_arrow);
            if(p!=string::npos && all_digits_spaces(s, p)){
                s.erase(0, p + utf_arrow.size());
                while(!s.empty() && s[0]==' ') s.erase(0,1);
                return;
            }
            size_t q = s.find("->");
            if(q!=string::npos && all_digits_spaces(s, q)){
                s.erase(0, q + 2);
                while(!s.empty() && s[0]==' ') s.erase(0,1);
                return;
            }
        };
        strip_prefix(line);
        vector<string> tk = tokenize(line);
        if(tk.empty()) continue; // no output
        string cmd = tk[0];
        if(cmd=="quit" || cmd=="exit"){ break; }
        auto invalid = [&](){ cout << "Invalid\n"; };

        if(cmd=="su"){
            if(tk.size()<2 || tk.size()>3 || !is_valid_userid(tk[1])){ invalid(); continue; }
            string uid=tk[1]; string pwd = tk.size()==3? tk[2]: string();
            Account* a = find_account(accounts, uid);
            if(!a){ invalid(); continue; }
            int cur = current_priv(stack);
            bool can_omit = (cur > a->privilege);
            if(tk.size()==2 && !can_omit){ invalid(); continue; }
            if(tk.size()==3){ if(!is_valid_password(pwd) || pwd!=a->password){ invalid(); continue; } }
            stack.push_back({a->userid, a->privilege, ""});
            save_login(stack);
            continue;
        }else if(cmd=="logout"){
            if(tk.size()!=1){ invalid(); continue; }
            if(stack.empty()){ invalid(); continue; }
            stack.pop_back();
            save_login(stack);
            continue;
        }else if(cmd=="register"){
            if(tk.size()!=4 || !is_valid_userid(tk[1]) || !is_valid_password(tk[2]) || !is_valid_username(tk[3])){ invalid(); continue; }
            if(find_account(accounts, tk[1])){ invalid(); continue; }
            accounts.push_back({tk[1], tk[2], tk[3], 1});
            save_accounts(accounts);
            continue;
        }else if(cmd=="passwd"){
            if(tk.size()<3 || tk.size()>4 || !is_valid_userid(tk[1])){ invalid(); continue; }
            Account* a = find_account(accounts, tk[1]); if(!a){ invalid(); continue; }
            int cur = current_priv(stack);
            if(cur<1){ invalid(); continue; }
            if(cur==7){
                string newpwd = tk.back(); if(!is_valid_password(newpwd)){ invalid(); continue; }
                if(tk.size()==4){ if(!is_valid_password(tk[2]) || tk[2]!=a->password){ invalid(); continue; } }
                a->password = newpwd; save_accounts(accounts);
            }else{
                if(tk.size()!=4 || !is_valid_password(tk[2]) || tk[2]!=a->password || !is_valid_password(tk[3])){ invalid(); continue; }
                a->password = tk[3]; save_accounts(accounts);
            }
            continue;
        }else if(cmd=="useradd"){
            if(tk.size()!=5 || !is_valid_userid(tk[1]) || !is_valid_password(tk[2]) || !is_valid_priv(tk[3]) || !is_valid_username(tk[4])){ invalid(); continue; }
            int cur = current_priv(stack); if(cur<3){ invalid(); continue; }
            int pr = stoi(tk[3]); if(pr>=cur){ invalid(); continue; }
            if(find_account(accounts, tk[1])){ invalid(); continue; }
            accounts.push_back({tk[1], tk[2], tk[4], pr}); save_accounts(accounts);
            continue;
        }else if(cmd=="delete"){
            if(tk.size()!=2 || !is_valid_userid(tk[1])){ invalid(); continue; }
            int cur = current_priv(stack); if(cur<7){ invalid(); continue; }
            Account* a = find_account(accounts, tk[1]); if(!a){ invalid(); continue; }
            bool logged=false; for(auto &s: stack) if(s.userid==tk[1]) { logged=true; break; }
            if(logged){ invalid(); continue; }
            vector<Account> nv; nv.reserve(accounts.size());
            for(auto &x: accounts) if(x.userid!=tk[1]) nv.push_back(x);
            accounts.swap(nv); save_accounts(accounts);
            continue;
        }else if(cmd=="select"){
            if(tk.size()!=2){ invalid(); continue; }
            int cur = current_priv(stack); if(cur<3){ invalid(); continue; }
            if(stack.empty()){ invalid(); continue; }
            string isbn = tk[1]; if(!is_valid_isbn(isbn)){ invalid(); continue; }
            Book* b = find_book(books, isbn);
            if(!b){ books.push_back(Book{isbn, "", "", "", 0.0, 0}); save_books(books); }
            stack.back().selected_isbn = isbn;
            save_login(stack);
            continue;
        }else if(cmd=="modify"){
            int cur = current_priv(stack); if(cur<3){ invalid(); continue; }
            if(stack.empty() || stack.back().selected_isbn.empty()){ invalid(); continue; }
            Book* b = find_book(books, stack.back().selected_isbn);
            if(!b){ invalid(); continue; }
            set<string> keys;
            // Prepare new values
            string new_isbn=b->isbn, new_name=b->name, new_author=b->author, new_keyword=b->keyword; double new_price=b->price;
            for(size_t i=1;i<tk.size();++i){
                string p=tk[i];
                if(p.rfind("-ISBN=",0)==0){ if(!keys.insert("ISBN").second){ invalid(); goto apply_end; }
                    string val = p.substr(6);
                    if(val.empty() || !is_valid_isbn(val)){ invalid(); goto apply_end; }
                    if(val==b->isbn){ invalid(); goto apply_end; }
                    // cannot change to existing other ISBN
                    Book* other = find_book(books, val); if(other){ invalid(); goto apply_end; }
                    new_isbn = val;
                } else if(p.rfind("-name=\"",0)==0){ if(!keys.insert("name").second){ invalid(); goto apply_end; }
                    if(p.back()!='\"'){ invalid(); goto apply_end; }
                    string val = p.substr(7, p.size()-8);
                    if(val.empty() || !is_valid_named_field(val)){ invalid(); goto apply_end; }
                    new_name = val;
                } else if(p.rfind("-author=\"",0)==0){ if(!keys.insert("author").second){ invalid(); goto apply_end; }
                    if(p.back()!='\"'){ invalid(); goto apply_end; }
                    string val = p.substr(9, p.size()-10);
                    if(val.empty() || !is_valid_named_field(val)){ invalid(); goto apply_end; }
                    new_author = val;
                } else if(p.rfind("-keyword=\"",0)==0){ if(!keys.insert("keyword").second){ invalid(); goto apply_end; }
                    if(p.back()!='\"'){ invalid(); goto apply_end; }
                    string val = p.substr(10, p.size()-11);
                    if(val.empty() || !is_valid_keyword_all(val)){ invalid(); goto apply_end; }
                    if(!has_unique_keyword_segments(val)){ invalid(); goto apply_end; }
                    new_keyword = val;
                } else if(p.rfind("-price=",0)==0){ if(!keys.insert("price").second){ invalid(); goto apply_end; }
                    string val = p.substr(7);
                    if(val.empty()){ invalid(); goto apply_end; }
                    // allow digits and '.' only
                    int dots=0; for(char c: val){ if(!(isdigit((unsigned char)c) || c=='.')){ invalid(); goto apply_end; } dots+=(c=='.'); }
                    if(dots>1){ invalid(); goto apply_end; }
                    try{ new_price = stod(val); if(new_price<0){ invalid(); goto apply_end; } }catch(...){ invalid(); goto apply_end; }
                } else { invalid(); goto apply_end; }
            }
            // apply
            b->isbn = new_isbn; b->name = new_name; b->author = new_author; b->keyword = new_keyword; b->price = new_price;
            // if ISBN changed, also update selection
            stack.back().selected_isbn = b->isbn;
            save_books(books); save_login(stack);
            apply_end:;
            continue;
        }else if(cmd=="import"){
            int cur = current_priv(stack); if(cur<3){ invalid(); continue; }
            if(stack.empty() || stack.back().selected_isbn.empty() || tk.size()!=3){ invalid(); continue; }
            Book* b = find_book(books, stack.back().selected_isbn); if(!b){ invalid(); continue; }
            string qty = tk[1], cost = tk[2];
            if(!is_positive_integer(qty) || !is_positive_number(cost)){ invalid(); continue; }
            long long q = stoll(qty);
            b->stock += q; save_books(books);
            continue;
        }else if(cmd=="buy"){
            int cur = current_priv(stack); if(cur<1){ invalid(); continue; }
            if(tk.size()!=3){ invalid(); continue; }
            string isbn=tk[1], qty=tk[2]; if(!is_valid_isbn(isbn) || !is_positive_integer(qty)){ invalid(); continue; }
            Book* b = find_book(books, isbn); if(!b){ invalid(); continue; }
            long long q = stoll(qty); if(q<=0){ invalid(); continue; }
            if(b->stock < q){ invalid(); continue; }
            b->stock -= q; save_books(books);
            double total = b->price * (double)q;
            cout.setf(std::ios::fixed); cout << setprecision(2) << total << "\n";
            continue;
        }else if(cmd=="show"){
            int cur = current_priv(stack); if(cur<1){ invalid(); continue; }
            vector<Book> out;
            if(tk.size()==1){ out = books; }
            else if(tk.size()==2){
                string p=tk[1];
                if(p.rfind("-ISBN=",0)==0){ string val=p.substr(6); if(val.empty()){ invalid(); continue; } for(auto &b: books) if(b.isbn==val) out.push_back(b); }
                else if(p.rfind("-name=\"",0)==0){ if(p.back()!='\"'){ invalid(); continue; } string val=p.substr(7, p.size()-8); if(val.empty()){ invalid(); continue; } for(auto &b: books) if(b.name==val) out.push_back(b); }
                else if(p.rfind("-author=\"",0)==0){ if(p.back()!='\"'){ invalid(); continue; } string val=p.substr(9, p.size()-10); if(val.empty()){ invalid(); continue; } for(auto &b: books) if(b.author==val) out.push_back(b); }
                else if(p.rfind("-keyword=\"",0)==0){ if(p.back()!='\"'){ invalid(); continue; } string val=p.substr(10, p.size()-11); if(val.empty()){ invalid(); continue; } if(val.find('|')!=string::npos){ invalid(); continue; }
                    // match any book containing this keyword segment
                    for(auto &b: books){ size_t start=0; bool ok=false; while(true){ size_t pos=b.keyword.find('|', start); string seg=b.keyword.substr(start, pos==string::npos? string::npos : pos-start); if(seg==val){ ok=true; break; } if(pos==string::npos) break; start=pos+1; } if(ok) out.push_back(b); }
                }
                else { invalid(); continue; }
            } else { invalid(); continue; }
            sort(out.begin(), out.end(), [](const Book& a, const Book& b){ return a.isbn < b.isbn; });
            if(out.empty()){ cout << "\n"; continue; }
            for(const auto &b: out){ cout << b.isbn << '\t' << b.name << '\t' << b.author << '\t' << b.keyword << '\t' << fixed << setprecision(2) << b.price << '\t' << b.stock << "\n"; }
            continue;
        }else if(cmd=="log" || cmd=="report"){
            invalid(); continue;
        }else{
            invalid(); continue;
        }
    }
    return 0;
}
