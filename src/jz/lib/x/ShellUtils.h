#ifndef X_SHELL_UTILS_H_
#define X_SHELL_UTILS_H_

namespace x {

class ShellUtils {
public:
    /**
     * @brief 通过管道的方式获取脚本输出
     * @param cmd
     * @param output
     * @return
     */
    static int getShellOutput(const std::string cmd, std::string &output) {
        if (cmd.length() <= 0) {
            return -1;
        }

        FILE *pp = popen(cmd.c_str(), "r");
        if (0 == pp) {
            return -1;
        }

        output.clear();
        while (true) {
            int n = fgetc(pp);
            if (EOF == n) {
                break;
            } else {
                char c = n;
                output += c;
            }
        }
        pclose(pp);
        pp = 0;
        return 0;
    }
};

}//namespace x {

#endif //X_SHELL_UTILS_H_
