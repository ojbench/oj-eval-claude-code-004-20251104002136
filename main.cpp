#include <bits/stdc++.h>
using namespace std;

// Minimal implementation focusing on Account System to pass some tests.
// Data persistence via simple files under ./data/ using <= 20 files total.
// This is a staged implementation; later iterations will add Book and Log systems.

struct Account {
    string userid;
    string password;
    string username;
    int privilege; // 1,3,7
};

static const string ACC_FILE = "accounts.dat";
static const string LOGIN_FILE = "login.dat"; // ephemeral, but we will write to respect persistence guideline minimally

// Utilities
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

// Persistent storage of accounts: plain CSV lines
static vector<Account> load_accounts(){
    vector<Account> v;
    ifstream fin(ACC_FILE);
    if(!fin){
        // first run: create root
        Account root{"root","sjtu","root",7};
        v.push_back(root);
        ofstream fout(ACC_FILE);
        fout << root.userid << "," << root.password << "," << root.username << "," << root.privilege << "\n";
        fout.close();
        return v;
    }
    string line;
    while(getline(fin,line)){
        if(line.empty()) continue;
        stringstream ss(line);
        string u,p,n,pr;
        if(!getline(ss,u,',')) continue;
        if(!getline(ss,p,',')) continue;
        if(!getline(ss,n,',')) continue;
        if(!getline(ss,pr,',')) continue;
        Account a{u,p,n,stoi(pr)};
        v.push_back(a);
    }
    return v;
}

static void save_accounts(const vector<Account>& v){
    ofstream fout(ACC_FILE, ios::trunc);
    for(const auto &a: v){
        fout << a.userid << "," << a.password << "," << a.username << "," << a.privilege << "\n";
    }
}

struct Session { string userid; int privilege; string selected_isbn; };
static vector<Session> load_login(){
    vector<Session> s;
    ifstream fin(LOGIN_FILE);
    string line;
    while(getline(fin,line)){
        if(line.empty()) continue;
        stringstream ss(line);
        string u,pr,sel;
        getline(ss,u,','); getline(ss,pr,','); getline(ss,sel,',');
        if(!pr.empty()) s.push_back({u, stoi(pr), sel});
    }
    return s;
}
static void save_login(const vector<Session>& s){
    ofstream fout(LOGIN_FILE, ios::trunc);
    for(auto &x: s){
        fout << x.userid << "," << x.privilege << "," << x.selected_isbn << "\n";
    }
}

static int current_priv(const vector<Session>& stack){
    if(stack.empty()) return 0; else return stack.back().privilege;
}
static Account* find_account(vector<Account>& v, const string& uid){
    for(auto &a: v) if(a.userid==uid) return &a; return nullptr;
}

static vector<string> tokenize(const string &line){
    vector<string> t;
    int i=0,n=line.size();
    while(i<n){
        while(i<n && line[i]==' ') i++;
        if(i>=n) break;
        if(line[i]=='\"'){
            i++; string buf; bool closed=false;
            while(i<n){ if(line[i]=='\"'){ closed=true; i++; break;} buf.push_back(line[i++]); }
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
    vector<Session> stack = {}; // do not load previous sessions; fresh run

    string line;
    while(true){
        if(!getline(cin,line)) break;
        // normalize spaces: multiple spaces treated as single, leading/trailing ignored
        // Already handled by tokenizer
        vector<string> tk = tokenize(line);
        if(tk.empty()) continue; // empty command produces no output
        string cmd = tk[0];
        if(cmd=="quit" || cmd=="exit"){ break; }
        auto invalid = [&](){ cout << "Invalid\n"; };

        if(cmd=="su"){
            // su [UserID] ([Password])?
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
            // register [UserID] [Password] [Username]  -> creates privilege 1
            if(tk.size()!=4 || !is_valid_userid(tk[1]) || !is_valid_password(tk[2]) || !is_valid_username(tk[3])){ invalid(); continue; }
            if(find_account(accounts, tk[1])){ invalid(); continue; }
            accounts.push_back({tk[1], tk[2], tk[3], 1});
            save_accounts(accounts);
            continue;
        }else if(cmd=="passwd"){
            // passwd [UserID] ([CurrentPassword])? [NewPassword]
            if(tk.size()<3 || tk.size()>4 || !is_valid_userid(tk[1])){ invalid(); continue; }
            Account* a = find_account(accounts, tk[1]); if(!a){ invalid(); continue; }
            int cur = current_priv(stack);
            if(cur<1){ invalid(); continue; }
            if(cur==7){
                // can omit current password
                string newpwd = tk.back(); if(!is_valid_password(newpwd)){ invalid(); continue; }
                if(tk.size()==4){ // provided current password; must match
                    if(!is_valid_password(tk[2]) || tk[2]!=a->password){ invalid(); continue; }
                }
                a->password = newpwd; save_accounts(accounts);
            }else{
                if(tk.size()!=4 || !is_valid_password(tk[2]) || tk[2]!=a->password || !is_valid_password(tk[3])){ invalid(); continue; }
                a->password = tk[3]; save_accounts(accounts);
            }
            continue;
        }else if(cmd=="useradd"){
            // useradd [UserID] [Password] [Privilege] [Username]
            if(tk.size()!=5 || !is_valid_userid(tk[1]) || !is_valid_password(tk[2]) || !is_valid_priv(tk[3]) || !is_valid_username(tk[4])){ invalid(); continue; }
            int cur = current_priv(stack); if(cur<3){ invalid(); continue; }
            int pr = stoi(tk[3]); if(pr>=cur){ invalid(); continue; }
            if(find_account(accounts, tk[1])){ invalid(); continue; }
            accounts.push_back({tk[1], tk[2], tk[4], pr}); save_accounts(accounts);
            continue;
        }else if(cmd=="delete"){
            // delete [UserID]
            if(tk.size()!=2 || !is_valid_userid(tk[1])){ invalid(); continue; }
            int cur = current_priv(stack); if(cur<7){ invalid(); continue; }
            Account* a = find_account(accounts, tk[1]); if(!a){ invalid(); continue; }
            // if account logged in, fail
            bool logged=false; for(auto &s: stack) if(s.userid==tk[1]) { logged=true; break; }
            if(logged){ invalid(); continue; }
            // delete
            vector<Account> nv; nv.reserve(accounts.size());
            for(auto &x: accounts) if(x.userid!=tk[1]) nv.push_back(x);
            accounts.swap(nv); save_accounts(accounts);
            continue;
        }else if(cmd=="show" || cmd=="buy" || cmd=="select" || cmd=="modify" || cmd=="import" || cmd=="show" || cmd=="log" || cmd=="report"){
            // Not implemented yet
            invalid(); continue;
        }else{
            // commands containing only spaces are legal and produce no output handled earlier; others invalid
            invalid(); continue;
        }
    }
    return 0;
}
