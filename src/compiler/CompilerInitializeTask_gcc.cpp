#include <cassert>
#include <cctype>

#include "hscpp/compiler/CompilerInitializeTask_gcc.h"
#include "hscpp/Log.h"

namespace hscpp
{

    CompilerInitializeTask_gcc::CompilerInitializeTask_gcc(CompilerConfig* pConfig)
        : m_pConfig(pConfig)
    {}

    void CompilerInitializeTask_gcc::Start(ICmdShell *pCmdShell,
                                           std::chrono::milliseconds timeout,
                                           const std::function<void(Result)>& doneCb)
    {
        m_pCmdShell = pCmdShell;
        m_DoneCb = doneCb;

        m_StartTime = std::chrono::steady_clock::now();
        m_Timeout = timeout;

        std::string versionCmd = "\"" + m_pConfig->executable.u8string() + "\" --version";
        m_pCmdShell->StartTask(versionCmd, static_cast<int>(CompilerTask::GetVersion));
    }

    bool CompilerInitializeTask_gcc::StartNinja()
    {
        if (m_pConfig->ninjaExecutable.empty()) return false;

        m_StartTime = std::chrono::steady_clock::now();
        m_pCmdShell->Clear();

        std::string versionCmd = "\"" + m_pConfig->ninjaExecutable.u8string() + "\" --version";
        m_pCmdShell->StartTask(versionCmd, static_cast<int>(CompilerTask::GetNinjaVersion));

        return true;
    }

    void CompilerInitializeTask_gcc::Update()
    {
        int taskId = 0;
        ICmdShell::TaskState state = m_pCmdShell->Update(taskId);

        switch (state)
        {
            case ICmdShell::TaskState::Running:
            {
                auto now = std::chrono::steady_clock::now();
                if (now - m_StartTime > m_Timeout)
                {
                    m_pCmdShell->CancelTask();
                    TriggerDoneCb(Result::Failure);
                    return;
                }

                break;
            }
            case ICmdShell::TaskState::Idle:
            case ICmdShell::TaskState::Cancelled:
                // Do nothing.
                break;
            case ICmdShell::TaskState::Done:
                HandleTaskComplete(static_cast<CompilerTask>(taskId));
                break;
            case ICmdShell::TaskState::Error:
                log::Error() << HSCPP_LOG_PREFIX
                    << "CmdShell task '" << taskId << "' resulted in error." << log::End();
                TriggerDoneCb(Result::Failure);
                return;
            default:
                assert(false);
                break;
        }
    }

    void CompilerInitializeTask_gcc::TriggerDoneCb(Result result)
    {
        if (m_DoneCb != nullptr)
        {
            m_DoneCb(result);
            m_pCmdShell->Clear();
        }

        m_DoneCb = nullptr;
    }

    void CompilerInitializeTask_gcc::HandleTaskComplete(CompilerTask task)
    {
        const std::vector<std::string>& output = m_pCmdShell->PeekTaskOutput();

        switch (task)
        {
            case CompilerTask::GetVersion:
                if (HandleGetVersionTaskComplete(output))
                {
                    if (! StartNinja())
                    {
                        TriggerDoneCb(Result::Success);
                    }
                }
                else
                {
                    TriggerDoneCb(Result::Failure);
                }
                break;
            case CompilerTask::GetNinjaVersion:
                if (HandleGetNinjaVersionTaskComplete(output))
                {
                    // Ninja check if optional.
                }
                TriggerDoneCb(Result::Success);
                break;

            default:
                assert(false);
                break;
        }
    }

    bool CompilerInitializeTask_gcc::IsOutputHasValidVersion(const std::vector<std::string>& output, bool hasBanner)
    {
        // Very rudimentary verification; assume that a --version string will have at least
        // a letter, a number, and a period associated with it.
        bool bSawLetter = false;
        bool bSawNumber = false;
        bool bSawPeriod = false;
        bool bValidVersion = false;
        for (const auto& line : output)
        {
            for (const char c : line)
            {
                if (std::isdigit(c))
                {
                    bSawNumber = true;
                }

                if (std::isalpha(c))
                {
                    bSawLetter = true;
                }

                if (c == '.')
                {
                    bSawPeriod = true;
                }

                if (bSawNumber && bSawPeriod && (!hasBanner || bSawLetter))
                {
                    bValidVersion = true;
                    break;
                }
            }
        }

        return bValidVersion;
    }

    bool CompilerInitializeTask_gcc::HandleGetVersionTaskComplete(const std::vector<std::string>& output)
    {
        if (output.empty())
        {
            log::Error() << HSCPP_LOG_PREFIX << "Failed to get version for compiler '"
                << m_pConfig->executable.u8string() << log::End("'.");

            return false;
        }

        if (!IsOutputHasValidVersion(output, true))
        {
            log::Error() << HSCPP_LOG_PREFIX << "Failed to get version for compiler '"
                << m_pConfig->executable.u8string() << log::End("'.");

            return false;
        }

        // Since --version verification is not very robust, print out the discovered compiler.
        log::Info() << log::End(); // newline
        log::Info() << HSCPP_LOG_PREFIX << "Found compiler version:" << log::End();
        for (const auto& line : output)
        {
            log::Info() << "    " << line << log::End();
        }
        log::Info() << log::End();

        return true;
    }

    bool CompilerInitializeTask_gcc::HandleGetNinjaVersionTaskComplete(const std::vector<std::string>& output)
    {
        if (output.empty())
        {
            log::Error() << HSCPP_LOG_PREFIX << "Failed to get version for ninja '"
                << m_pConfig->ninjaExecutable.u8string() << log::End("'.");

            return false;
        }

        if (!IsOutputHasValidVersion(output, false))
        {
            log::Error() << HSCPP_LOG_PREFIX << "Failed to get version for ninja '"
                << m_pConfig->ninjaExecutable.u8string() << log::End("'.");

            return false;
        }

        log::Info() << log::End(); // newline
        log::Info() << HSCPP_LOG_PREFIX << "Found ninja version:" << log::End();
        for (const auto& line : output)
        {
            log::Info() << "    " << line << log::End();
        }
        log::Info() << log::End();

        return true;
    }

}