#include <bits/stdc++.h>
using namespace std;

/*
  MathSolutionInspector
  ---------------------
  High-level idea:
  - Teacher defines a simple linear equation in one variable: ax + b = c
  - Student writes step-by-step solution as text lines
  - Engine parses each step, checks:
      * Is the transformation valid?
      * Is there arithmetic error?
      * Is there algebraic / logical error?
      * Is the format broken (missing =, wrong structure, etc.)?
  - Reports:
      * First wrong step index
      * Type of error
      * Suggested correction / hint
*/

struct Equation {
    // Represents ax + b = c
    double a = 0, b = 0, c = 0;
};

struct ParsedStep {
    // Represents something like: "2x + 3 = 7"
    double a = 0, b = 0, c = 0;
    bool valid = false;
};

enum class ErrorType {
    NONE,
    ARITHMETIC,
    LOGIC,
    FORMAT
};

struct StepAnalysis {
    int stepIndex = -1;
    ErrorType type = ErrorType::NONE;
    string message;
};

static inline string trim(const string &s) {
    string t = s;
    t.erase(t.begin(), find_if(t.begin(), t.end(), [](unsigned char ch){ return !isspace(ch); }));
    t.erase(find_if(t.rbegin(), t.rend(), [](unsigned char ch){ return !isspace(ch); }).base(), t.end());
    return t;
}

// Very simple parser for expressions of form: ax + b
// Supports: "2x + 3", "-x + 5", "x - 4", "3x", "x", "5"
bool parseLinearExpr(const string &expr, double &a, double &b) {
    // Replace all spaces
    string s;
    for (char ch : expr) if (!isspace((unsigned char)ch)) s.push_back(ch);
    if (s.empty()) return false;

    // We will look for 'x'
    // Strategy:
    //  - If there's 'x', split into coefficient part and constant part
    //  - If no 'x', then a = 0, b = value
    size_t posX = s.find('x');
    if (posX == string::npos) {
        // No x → pure constant
        try {
            b = stod(s);
            a = 0;
            return true;
        } catch (...) {
            return false;
        }
    }

    // There is x
    string left = s.substr(0, posX);   // coefficient part
    string right = s.substr(posX + 1); // constant part (like +3, -4, etc.)

    // Coefficient
    if (left.empty() || left == "+") {
        a = 1;
    } else if (left == "-") {
        a = -1;
    } else {
        try {
            a = stod(left);
        } catch (...) {
            return false;
        }
    }

    // Constant
    if (right.empty()) {
        b = 0;
        return true;
    } else {
        try {
            b = stod(right);
            return true;
        } catch (...) {
            return false;
        }
    }
}

// Parse equation of form: "2x + 3 = 7"
ParsedStep parseEquation(const string &line) {
    ParsedStep ps;
    string s = trim(line);
    size_t posEq = s.find('=');
    if (posEq == string::npos) {
        ps.valid = false;
        return ps;
    }

    string left = trim(s.substr(0, posEq));
    string right = trim(s.substr(posEq + 1));

    double aL = 0, bL = 0, aR = 0, bR = 0;
    if (!parseLinearExpr(left, aL, bL)) {
        ps.valid = false;
        return ps;
    }
    if (!parseLinearExpr(right, aR, bR)) {
        ps.valid = false;
        return ps;
    }

    // Bring everything to left: (aL - aR)x + (bL - bR) = 0
    ps.a = aL - aR;
    ps.b = bL - bR;
    ps.c = 0;
    ps.valid = true;
    return ps;
}

// Check if a step is a valid transformation from previous step
bool isValidTransformation(const ParsedStep &prev, const ParsedStep &curr) {
    // For a linear equation ax + b = 0, valid transformations include:
    // - Adding/subtracting same constant to both sides (b changes logically)
    // - Dividing both sides by non-zero constant (a, b scaled)
    // Here we approximate by checking if the solution set is the same.

    // If both represent same equation up to a non-zero scalar multiple → valid
    double a1 = prev.a, b1 = prev.b;
    double a2 = curr.a, b2 = curr.b;

    // Both zero?
    if (fabs(a1) < 1e-9 && fabs(b1) < 1e-9 && fabs(a2) < 1e-9 && fabs(b2) < 1e-9)
        return true;

    // If one is zero and other not → not equivalent
    if ((fabs(a1) < 1e-9 && fabs(b1) >= 1e-9) || (fabs(a2) < 1e-9 && fabs(b2) >= 1e-9))
        return false;

    // Check proportionality: a2 = k*a1, b2 = k*b1
    if (fabs(a1) < 1e-9 && fabs(a2) < 1e-9) {
        // Both have no x term → compare constants
        return fabs(b1 - b2) < 1e-9;
    }

    double k = a2 / (a1 == 0 ? 1e-9 : a1);
    if (fabs(b1 * k - b2) < 1e-6) return true;
    return false;
}

// Analyze a sequence of steps
StepAnalysis analyzeSteps(const Equation &correctEq, const vector<string> &steps) {
    StepAnalysis result;
    result.type = ErrorType::NONE;
    result.stepIndex = -1;
    result.message = "No errors detected. Great job!";

    if (steps.empty()) {
        result.type = ErrorType::FORMAT;
        result.stepIndex = 0;
        result.message = "No steps provided.";
        return result;
    }

    // Parse teacher's original equation as reference
    // Represented as ax + b = 0
    ParsedStep correct;
    correct.a = correctEq.a;
    correct.b = correctEq.b;
    correct.c = 0;
    correct.valid = true;

    ParsedStep prev = correct;
    for (int i = 0; i < (int)steps.size(); ++i) {
        string line = steps[i];
        ParsedStep curr = parseEquation(line);

        if (!curr.valid) {
            result.type = ErrorType::FORMAT;
            result.stepIndex = i;
            result.message = "Step " + to_string(i+1) + " has invalid format. Make sure you use '=' and a linear expression.";
            return result;
        }

        // Check logical equivalence
        if (!isValidTransformation(prev, curr)) {
            // Try to guess if it's arithmetic or logic
            // If structure looks similar but constants off → arithmetic
            bool maybeArithmetic = false;
            if (fabs(prev.a - curr.a) < 1e-6 && fabs(prev.b - curr.b) > 1e-3) {
                maybeArithmetic = true;
            }

            result.stepIndex = i;
            if (maybeArithmetic) {
                result.type = ErrorType::ARITHMETIC;
                result.message =
                    "Step " + to_string(i+1) +
                    " seems to have an arithmetic mistake. Check your addition/subtraction or multiplication.";
            } else {
                result.type = ErrorType::LOGIC;
                result.message =
                    "Step " + to_string(i+1) +
                    " is not logically equivalent to the previous one. Recheck the algebraic transformation.";
            }
            return result;
        }

        prev = curr;
    }

    // If last step is something like x = value, we can check correctness
    ParsedStep last = parseEquation(steps.back());
    if (fabs(last.a) < 1e-9 && fabs(last.b) < 1e-9) {
        // 0 = 0 → infinite solutions or identity
        result.message = "All steps are valid. Final result suggests an identity (infinitely many solutions).";
        return result;
    }

    if (fabs(last.a) < 1e-9 && fabs(last.b) >= 1e-9) {
        // 0x + b = 0 → b = 0 or contradiction
        if (fabs(last.b) < 1e-9) {
            result.message = "All steps are valid. Final result suggests an identity.";
        } else {
            result.message = "All steps are valid. Final result suggests no solution (contradiction).";
        }
        return result;
    }

    // Solve correct equation and student's final equation
    double x_correct = ( -correct.b ) / (correct.a == 0 ? 1e-9 : correct.a );
    double x_student = ( -last.b ) / (last.a == 0 ? 1e-9 : last.a );

    if (fabs(x_correct - x_student) < 1e-6) {
        result.message = "All steps are valid and the final solution for x is correct.";
    } else {
        result.type = ErrorType::ARITHMETIC;
        result.stepIndex = (int)steps.size() - 1;
        result.message =
            "Your transformations are mostly valid, but the final numeric value of x is incorrect. Recheck the last step.";
    }

    return result;
}

// Helper to build Equation from teacher form: ax + b = c
Equation buildEquation(double a, double b, double c) {
    Equation eq;
    // Bring to ax + b - c = 0 → ax + (b - c) = 0
    eq.a = a;
    eq.b = b - c;
    eq.c = 0;
    return eq;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    cout << "=== MathSolutionInspector (C++ Core Engine) ===\n\n";

    // Example: teacher equation: 2x + 3 = 7
    // Correct solution: 2x = 4 → x = 2
    double A, B, C;
    cout << "Enter teacher equation in the form ax + b = c\n";
    cout << "a: "; cin >> A;
    cout << "b: "; cin >> B;
    cout << "c: "; cin >> C;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    Equation eq = buildEquation(A, B, C);

    cout << "\nNow enter student's steps (one per line). Empty line to finish:\n";
    vector<string> steps;
    while (true) {
        string line;
        getline(cin, line);
        if (trim(line).empty()) break;
        steps.push_back(line);
    }

    if (steps.empty()) {
        cout << "\nNo steps provided. Nothing to analyze.\n";
        return 0;
    }

    StepAnalysis analysis = analyzeSteps(eq, steps);

    cout << "\n=== Analysis Result ===\n";
    if (analysis.type == ErrorType::NONE) {
        cout << "Status: OK\n";
        cout << "Message: " << analysis.message << "\n";
    } else {
        cout << "Status: ERROR DETECTED\n";
        cout << "First wrong step: #" << (analysis.stepIndex + 1) << "\n";
        cout << "Error type: ";
        switch (analysis.type) {
            case ErrorType::ARITHMETIC: cout << "Arithmetic error\n"; break;
            case ErrorType::LOGIC:      cout << "Logical/algebraic error\n"; break;
            case ErrorType::FORMAT:     cout << "Format/structure error\n"; break;
            default:                    cout << "Unknown\n"; break;
        }
        cout << "Details: " << analysis.message << "\n";
    }

    cout << "\nHint: You can extend this engine to:\n";
    cout << "- Batch‑analyze thousands of solutions.\n";
    cout << "- Export reports for teachers.\n";
    cout << "- Integrate with a web or desktop UI.\n";

    return 0;
}